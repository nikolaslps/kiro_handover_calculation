#pragma once

#include <rclcpp/rclcpp.hpp>

#include <hri_msgs/msg/ids_list.hpp>
#include <hri_msgs/msg/skeleton2_d.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include "std_msgs/msg/bool.hpp"

#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

#include "kiro_handover_calculation/body_selector.hpp"
#include "kiro_handover_interfaces/srv/get_active_body_id.hpp"
#include "kiro_handover_interfaces/srv/activate_handover.hpp"

#include <lifecycle_msgs/srv/change_state.hpp>

#include <map>
#include <set>
#include <string>

namespace kiro_handover_calculation
{

class OptimalVolumeCalculator : public rclcpp::Node
{
public:
    OptimalVolumeCalculator();

private:
    void handleActivateHandover(
        const std::shared_ptr<kiro_handover_interfaces::srv::ActivateHandover::Request> request,
        std::shared_ptr<kiro_handover_interfaces::srv::ActivateHandover::Response> response);
    void handleGetActiveBodyID(
        const std::shared_ptr<kiro_handover_interfaces::srv::GetActiveBodyID::Request>,
        std::shared_ptr<kiro_handover_interfaces::srv::GetActiveBodyID::Response> response);

    void callbackTrackedBodies(const hri_msgs::msg::IdsList::SharedPtr msg);
    void callbackJointStates(const sensor_msgs::msg::JointState::SharedPtr msg);
    void callbackSkeleton2D(const hri_msgs::msg::Skeleton2D::SharedPtr msg);

    void computeHandoverVolume(const hri_msgs::msg::Skeleton2D::SharedPtr msg);

    void switchToBody(const std::string& new_id);
    std::string selectBodyId() const;

private:
    std::string camera_frame_;

    std::string active_body_id_;
    std::map<std::string, double> upper_arm_lengths_;

    std::set<std::string> currently_tracked_bodies_;
    rclcpp::Time last_body_seen_;

    bool activate_calc = false; // whether the handover calculation module is active

    // TF2
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    // Helpers
    BodySelector selector_;

    // Subscribers
    rclcpp::Subscription<hri_msgs::msg::IdsList>::SharedPtr tracked_bodies_sub_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_states_sub_;
    rclcpp::Subscription<hri_msgs::msg::Skeleton2D>::SharedPtr skeleton2d_sub_;

    // Publisher
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr handover_volume_pub_;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr status_pub_;

    // Service
    rclcpp::Service<kiro_handover_interfaces::srv::GetActiveBodyID>::SharedPtr get_body_id_service_;
    rclcpp::Service<kiro_handover_interfaces::srv::ActivateHandover>::SharedPtr activate_service_;
    rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr hri_lifecycle_client_;
};

}  // namespace kiro_handover_calculation