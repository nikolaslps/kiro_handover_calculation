#include "kiro_handover_calculation/optimal_volume_calculator.hpp"

#include "kiro_handover_calculation/arm_fk.hpp"

#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <visualization_msgs/msg/marker.hpp>

#include <tf2/LinearMath/Vector3.h>
#include <tf2/exceptions.h>
#include <tf2/time.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace kiro_handover_calculation
{

OptimalVolumeCalculator::OptimalVolumeCalculator() : rclcpp::Node("optimal_volume_calculator"),
    tf_buffer_(std::make_shared<tf2_ros::Buffer>(this->get_clock())),
    tf_listener_(std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this)),
    selector_(tf_buffer_, this->declare_parameter<std::string>("camera_frame", "camera_color_optical_frame")) // default camera frame
{
    activate_calc = false; 

    auto qos = rclcpp::QoS(rclcpp::KeepLast(10))
                .reliability(rclcpp::ReliabilityPolicy::Reliable)
                .durability(rclcpp::DurabilityPolicy::Volatile);

    camera_frame_ = this->get_parameter("camera_frame").as_string();
    last_body_seen_ = this->now();

    // Service to activate lifecycle of HRI body detect module
    hri_lifecycle_client_ = this->create_client<lifecycle_msgs::srv::ChangeState>("/hri_body_detect/change_state");

    // Service to start the calculations
    activate_service_ = this->create_service<kiro_handover_interfaces::srv::ActivateHandover>(
        "/activate_handover_calc",
        std::bind(&OptimalVolumeCalculator::handleActivateHandover, this, std::placeholders::_1, std::placeholders::_2));

    // Service for active body id
    get_body_id_service_ = this->create_service<kiro_handover_interfaces::srv::GetActiveBodyID>(
        "/get_active_body_id",
        std::bind(&OptimalVolumeCalculator::handleGetActiveBodyID, this, std::placeholders::_1, std::placeholders::_2));

    // Publisher to indicate if handover phase is active (the header fixer needs that to start image streaming)
    status_pub_ = this->create_publisher<std_msgs::msg::Bool>("/handover_status", 10);

    // Tracked bodies subscription
    tracked_bodies_sub_ = this->create_subscription<hri_msgs::msg::IdsList>(
        "/humans/bodies/tracked", qos,
        std::bind(&OptimalVolumeCalculator::callbackTrackedBodies, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Optimal Volume Calculator initialized. Service: /get_active_body_id");
}

void OptimalVolumeCalculator::handleActivateHandover(
    const std::shared_ptr<kiro_handover_interfaces::srv::ActivateHandover::Request> request,
    std::shared_ptr<kiro_handover_interfaces::srv::ActivateHandover::Response> response)
{
    this->activate_calc = request->handover_phase;

    // send the boolean 'activate_calc' to header fixer
    auto status_msg = std_msgs::msg::Bool();
    status_msg.data = this->activate_calc;
    status_pub_->publish(status_msg);

    if (this->activate_calc) {
        auto conf_req = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
        conf_req->transition.id = 1; // CONFIGURE

        auto result_future = hri_lifecycle_client_->async_send_request(conf_req,
            [this](rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedFuture future) {
                auto result = future.get();
                if (result->success) {
                    RCLCPP_INFO(this->get_logger(), "Configuration successful, now activating...");
                    auto act_req = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
                    act_req->transition.id = 3; // ACTIVATE
                    this->hri_lifecycle_client_->async_send_request(act_req);
                }
            });

        RCLCPP_INFO(this->get_logger(), "Handover ENABLED: Configuring and Activating HRI body detect module...");
    } 
    else {
        this->activate_calc = false;
        this->switchToBody(""); // Clean up

        auto deact_req = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
        deact_req->transition.id = 4; // DEACTIVATE
        
        auto result_future = hri_lifecycle_client_->async_send_request(deact_req,
            [this](rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedFuture future) {
                auto result = future.get();
                if (result->success) {
                    RCLCPP_INFO(this->get_logger(), "Deactivation successful, now unconfiguring...");
                    auto unconf_req = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
                    unconf_req->transition.id = 2; // UNCONFIGURE
                    this->hri_lifecycle_client_->async_send_request(unconf_req);
                }
            });
        
        RCLCPP_INFO(this->get_logger(), "Handover DISABLED: Unconfiguring and Deactivating HRI body detect module...");
    }
    
    response->success = true;
    response->message = activate_calc ? "Pipeline Started" : "Pipeline Stopped";
}

void OptimalVolumeCalculator::handleGetActiveBodyID(
    const std::shared_ptr<kiro_handover_interfaces::srv::GetActiveBodyID::Request> /*req*/,
    std::shared_ptr<kiro_handover_interfaces::srv::GetActiveBodyID::Response> response)
{
    response->body_id = active_body_id_;
    response->success = !active_body_id_.empty();
}

void OptimalVolumeCalculator::switchToBody(const std::string& new_id)
{
    if (new_id == active_body_id_) return;

    active_body_id_ = new_id;
    RCLCPP_INFO(this->get_logger(), "Tracking body: %s", active_body_id_.c_str());

    // reset per-body interfaces
    joint_states_sub_.reset();
    skeleton2d_sub_.reset();
    handover_volume_pub_.reset();

    if (active_body_id_.empty()) return;

    joint_states_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
        std::string("/humans/bodies/" + active_body_id_ + "/joint_states"), 10,
        std::bind(&OptimalVolumeCalculator::callbackJointStates, this, std::placeholders::_1));

    skeleton2d_sub_ = this->create_subscription<hri_msgs::msg::Skeleton2D>(
        std::string("/humans/bodies/" + active_body_id_ + "/skeleton2d"), 10,
        std::bind(&OptimalVolumeCalculator::callbackSkeleton2D, this, std::placeholders::_1));

    handover_volume_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
        std::string("/humans/bodies/" + active_body_id_ + "/handover_volume"), 10);
}

void OptimalVolumeCalculator::callbackTrackedBodies(const hri_msgs::msg::IdsList::SharedPtr msg)
{
    if (!activate_calc) return;

    if (msg->ids.empty()) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000, "No bodies tracked");

        if (!active_body_id_.empty() && (this->now() - last_body_seen_).seconds() > 2.0) {
            switchToBody("");
        }
        return;
    }

    currently_tracked_bodies_.clear();
    for (const auto& id : msg->ids) currently_tracked_bodies_.insert(id);
    last_body_seen_ = this->now();

    selector_.setCameraFrame(camera_frame_);
    selector_.setTrackedBodies(currently_tracked_bodies_);

    const std::string selected = selector_.select(active_body_id_);
    switchToBody(selected);
}

void OptimalVolumeCalculator::callbackJointStates(const sensor_msgs::msg::JointState::SharedPtr /*msg*/)
{
    if (!activate_calc || active_body_id_.empty()) return;

    try {
        const std::string target_frame = "body_" + active_body_id_;
        const std::string l_shoulder_frame = std::string("l_y_shoulder_") + active_body_id_;
        const std::string l_elbow_frame = std::string("l_elbow_") + active_body_id_;
        const std::string r_shoulder_frame = std::string("r_y_shoulder_") + active_body_id_;
        const std::string r_elbow_frame = std::string("r_elbow_") + active_body_id_;

        auto tr_l_shoulder = tf_buffer_->lookupTransform(target_frame, l_shoulder_frame, tf2::TimePointZero);
        auto tr_l_elbow = tf_buffer_->lookupTransform(target_frame, l_elbow_frame, tf2::TimePointZero);
        auto tr_r_shoulder = tf_buffer_->lookupTransform(target_frame, r_shoulder_frame, tf2::TimePointZero);
        auto tr_r_elbow = tf_buffer_->lookupTransform(target_frame, r_elbow_frame, tf2::TimePointZero);

        tf2::Vector3 l_shoulder_pos(tr_l_shoulder.transform.translation.x,
                                    tr_l_shoulder.transform.translation.y,
                                    tr_l_shoulder.transform.translation.z);
        tf2::Vector3 l_elbow_pos(tr_l_elbow.transform.translation.x,
                                tr_l_elbow.transform.translation.y,
                                tr_l_elbow.transform.translation.z);
        tf2::Vector3 r_shoulder_pos(tr_r_shoulder.transform.translation.x,
                                    tr_r_shoulder.transform.translation.y,
                                    tr_r_shoulder.transform.translation.z);
        tf2::Vector3 r_elbow_pos(tr_r_elbow.transform.translation.x,
                                tr_r_elbow.transform.translation.y,
                                tr_r_elbow.transform.translation.z);

        const double left_upper_arm_length = (l_elbow_pos - l_shoulder_pos).length();
        const double right_upper_arm_length = (r_elbow_pos - r_shoulder_pos).length();
        upper_arm_lengths_[active_body_id_] = (left_upper_arm_length + right_upper_arm_length) / 2.0;
    } catch (const tf2::TransformException&) {
        return;
    }
}

void OptimalVolumeCalculator::callbackSkeleton2D(const hri_msgs::msg::Skeleton2D::SharedPtr msg)
{
    if (!activate_calc) return;

    computeHandoverVolume(msg);
}

void OptimalVolumeCalculator::computeHandoverVolume(const hri_msgs::msg::Skeleton2D::SharedPtr /*msg*/)
{
    if (active_body_id_.empty()) 
        return;
    if (!handover_volume_pub_) 
        return;

    const std::string target_frame = "body_" + active_body_id_;
    const std::string l_shoulder_frame = std::string("l_y_shoulder_") + active_body_id_;
    const std::string r_shoulder_frame = std::string("r_y_shoulder_") + active_body_id_;

    geometry_msgs::msg::TransformStamped tr_l_shoulder;
    geometry_msgs::msg::TransformStamped tr_r_shoulder;

    try {
        tr_l_shoulder = tf_buffer_->lookupTransform(target_frame, l_shoulder_frame, tf2::TimePointZero);
        tr_r_shoulder = tf_buffer_->lookupTransform(target_frame, r_shoulder_frame, tf2::TimePointZero);
    } catch (const tf2::TransformException&) {
        return;
    }

    tf2::Vector3 l_shoulder_pos(tr_l_shoulder.transform.translation.x,
                                tr_l_shoulder.transform.translation.y,
                                tr_l_shoulder.transform.translation.z);
    tf2::Vector3 r_shoulder_pos(tr_r_shoulder.transform.translation.x,
                                tr_r_shoulder.transform.translation.y,
                                tr_r_shoulder.transform.translation.z);

    // compute shoulder axis (perpendicular to torso, pointing outward)
    tf2::Vector3 shoulder_axis = l_shoulder_pos - r_shoulder_pos;
    shoulder_axis.setZ(0); // project to horizontal plane
    shoulder_axis.normalize();

    const tf2::Vector3 l_shoulder_axis(0, 1, 0); // Y-axis for left shoulder
    const tf2::Vector3 r_shoulder_axis(0, 1, 0); // Y-axis for right shoulder

    const double upper_arm_length = (upper_arm_lengths_.count(active_body_id_) > 0) ? upper_arm_lengths_[active_body_id_] : 0.30;
    const double forearm_length = 0.25;

    // trajectory params for ergonomic handover volume
    const double shoulder_angle_min = 0.0;                  // 0 degrees (parallel to torso)
    const double shoulder_angle_max = 20.0 * M_PI / 180.0;  // 20 degrees
    const double elbow_min = 60.0 * M_PI / 180.0;           // 60 degrees
    const double elbow_max = 100.0 * M_PI / 180.0;          // 100 degrees

    const int n_shoulder_steps = 10;    // steps between min and max shoulder angle
    const int n_elbow_steps = 15;       // steps along elbow trajectory

    // forward direction = perpendicular to shoulder axis
    tf2::Vector3 forward_dir = tf2::Vector3(0, 0, 1).cross(shoulder_axis);
    forward_dir.normalize();

    // ensure forward points in front (positive X direction in body frame)
    if (forward_dir.x() < 0) forward_dir = -forward_dir;

    const tf2::Vector3 offset = forward_dir * 0.3; // 30 cm forward from torso <----- change this based if you want the volume to be closer/further from the human ----->

    // store left and right arm volumes
    std::vector<geometry_msgs::msg::Point> all_points;
    std::vector<tf2::Vector3> left_arm_points;
    std::vector<tf2::Vector3> right_arm_points;

    // volume for left arm
    for (int s = 0; s <= n_shoulder_steps; ++s) {
        const double shoulder_angle = shoulder_angle_min + (shoulder_angle_max - shoulder_angle_min) * s / n_shoulder_steps;

        for (int e = 0; e <= n_elbow_steps; ++e) {
            const double elbow_angle = elbow_min + (elbow_max - elbow_min) * e / n_elbow_steps;

            tf2::Vector3 wrist = computeArmFK(l_shoulder_pos, l_shoulder_axis,
                                                shoulder_angle, elbow_angle,
                                                upper_arm_length, forearm_length);
            wrist += offset;
            left_arm_points.push_back(wrist);

            geometry_msgs::msg::Point p;
            p.x = wrist.x(); 
            p.y = wrist.y(); 
            p.z = wrist.z();
            all_points.push_back(p);
        }
    }

    // volume for right arm
    for (int s = 0; s <= n_shoulder_steps; ++s) {
        const double shoulder_angle = shoulder_angle_min + (shoulder_angle_max - shoulder_angle_min) * s / n_shoulder_steps;

        for (int e = 0; e <= n_elbow_steps; ++e) {
            const double elbow_angle = elbow_min + (elbow_max - elbow_min) * e / n_elbow_steps;

            tf2::Vector3 wrist = computeArmFK(r_shoulder_pos, r_shoulder_axis,
                                                shoulder_angle, elbow_angle,
                                                upper_arm_length, forearm_length);
            wrist += offset;
            right_arm_points.push_back(wrist);

            geometry_msgs::msg::Point p;
            p.x = wrist.x(); 
            p.y = wrist.y(); 
            p.z = wrist.z();
            all_points.push_back(p);
        }
    }

    // fill the space BETWEEN left and right arm volumes
    const int n_interarm_steps = 8;
    const size_t n = std::min(left_arm_points.size(), right_arm_points.size());
    for (size_t i = 0; i < n; ++i) {
        for (int t = 1; t < n_interarm_steps; ++t) {
            const double blend = static_cast<double>(t) / n_interarm_steps;
            const tf2::Vector3 interpolated = left_arm_points[i].lerp(right_arm_points[i], blend);

            geometry_msgs::msg::Point p;
            p.x = interpolated.x(); 
            p.y = interpolated.y(); 
            p.z = interpolated.z();
            all_points.push_back(p);
        }
    }

    if (all_points.empty()) return;

    visualization_msgs::msg::Marker volume_marker;
    volume_marker.header.frame_id = target_frame;
    volume_marker.header.stamp = this->get_clock()->now();
    volume_marker.ns = "handover_volume";
    volume_marker.type = visualization_msgs::msg::Marker::POINTS;
    volume_marker.scale.x = 0.015;
    volume_marker.scale.y = 0.015;
    volume_marker.color.r = 0.0f;
    volume_marker.color.g = 1.0f;
    volume_marker.color.b = 0.0f;
    volume_marker.color.a = 0.4f;
    volume_marker.points = all_points;

    visualization_msgs::msg::MarkerArray ma;
    ma.markers.push_back(volume_marker);
    handover_volume_pub_->publish(ma);
}

}  // namespace kiro_handover_calculation

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<kiro_handover_calculation::OptimalVolumeCalculator>();

    // Single-threaded avoids data races when switching subscriptions/publishers.
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}