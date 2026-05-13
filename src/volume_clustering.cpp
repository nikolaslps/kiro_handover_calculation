#include "kiro_handover_calculation/volume_clustering.hpp"

#include <numeric>
#include <algorithm>
#include <random>
#include <thread>

namespace kiro_handover_calculation
{

VolumeClusteringService::VolumeClusteringService() : Node("volume_clustering_service") {
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this);

    this->declare_parameter<std::string>("planning_frame", "kiro_base_link"); // default planning frame
    planning_frame_ = this->get_parameter("planning_frame").as_string();

    // this allows the service and the temporary sub to run in parallel
    callback_group_ = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);

    // init the Service
    service_ = this->create_service<kiro_handover_interfaces::srv::ClusterHandoverVolume>(
        "cluster_handover_volume",
        std::bind(&VolumeClusteringService::handleClusterRequest, this, std::placeholders::_1, std::placeholders::_2),
        rmw_qos_profile_services_default,
        callback_group_
    );

    RCLCPP_INFO(this->get_logger(), "Clustering Service Node Ready.");
}

// --- K-Means ---
std::vector<kMeansCluster> VolumeClusteringService::kMeansClustering(const std::vector<geometry_msgs::msg::Point>& points, 
                                                                        int k, 
                                                                        int max_iterations) 
{
    std::vector<kMeansCluster> clusters(k);
    if (points.empty() || k <= 0) return clusters;

    for (int i = 0; i < k; ++i) clusters[i].cluster_id = i;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, points.size() - 1);
    for (int i = 0; i < k; ++i) clusters[i].centroid = points[dis(gen)];

    for (int iter = 0; iter < max_iterations; ++iter) {
        for (auto& cluster : clusters) cluster.points.clear();

        for (const auto& p : points) {
            double min_dist = std::numeric_limits<double>::max();
            int nearest_cluster = 0;
            for (int i = 0; i < k; ++i) {
                double dx = p.x - clusters[i].centroid.x;
                double dy = p.y - clusters[i].centroid.y;
                double dz = p.z - clusters[i].centroid.z;
                double dist = dx * dx + dy * dy + dz * dz;
                if (dist < min_dist) { min_dist = dist; nearest_cluster = i; }
            }
            clusters[nearest_cluster].points.push_back(p);
        }

        bool centroids_changed = false;
        for (int i = 0; i < k; ++i) {
            if (clusters[i].points.empty()) continue;
            geometry_msgs::msg::Point new_centroid;
            new_centroid.x = 0; new_centroid.y = 0; new_centroid.z = 0;
            for (const auto& p : clusters[i].points) {
                new_centroid.x += p.x; new_centroid.y += p.y; new_centroid.z += p.z;
            }
            new_centroid.x /= clusters[i].points.size();
            new_centroid.y /= clusters[i].points.size();
            new_centroid.z /= clusters[i].points.size();

            double dx = new_centroid.x - clusters[i].centroid.x;
            double dy = new_centroid.y - clusters[i].centroid.y;
            double dz = new_centroid.z - clusters[i].centroid.z;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > 0.01) { // 1cm threshold
                centroids_changed = true;
            }
            clusters[i].centroid = new_centroid;
        }
        if (!centroids_changed) break;
    }
    return clusters;
}

// --- Service Handler ---
void VolumeClusteringService::handleClusterRequest(
    const std::shared_ptr<kiro_handover_interfaces::srv::ClusterHandoverVolume::Request> request,
    std::shared_ptr<kiro_handover_interfaces::srv::ClusterHandoverVolume::Response> response) 
{
    // extract body_id and k from the request
    std::string body_id = request->body_id;
    std::string topic_name = "/humans/bodies/" + body_id + "/handover_volume";

    RCLCPP_INFO(this->get_logger(), "Request for k=%d on body: %s", request->k, body_id.c_str());

    // temporary subscription to grab the latest data for the requested body_id's volume
    visualization_msgs::msg::MarkerArray::SharedPtr msg;
    bool got_data = false;

    // subscription options to use the reentrant group for multithreading
    rclcpp::SubscriptionOptions sub_options;
    sub_options.callback_group = callback_group_;

    auto temp_sub = this->create_subscription<visualization_msgs::msg::MarkerArray>(
        topic_name, 10, 
        [&msg, &got_data](const visualization_msgs::msg::MarkerArray::SharedPtr m) {
            msg = m;
            got_data = true;
        }, sub_options);

    // wait for up to 2 second for a message to arrive (subscription callback in the background)
    auto start_time = this->now();
    while (!got_data && (this->now() - start_time).seconds() < 2.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!got_data || msg->markers.empty() || msg->markers[0].points.empty()) {
        response->success = false;
        response->message = "Timeout or empty data on topic " + topic_name;
        return;
    }

    const auto& all_points = msg->markers[0].points;
    std::string source_frame = msg->markers[0].header.frame_id;
    std::string planning_frame = planning_frame_; // "kiro_base_link" or "base_link"

    int k = request->k;
    if (all_points.size() < (size_t)k) {
        k = all_points.size();
    }

    RCLCPP_INFO(this->get_logger(), "Request received. Clustering into k=%d", k);
    std::vector<kMeansCluster> clusters = kMeansClustering(all_points, k);

    response->clustered_points.header.frame_id = planning_frame;
    response->clustered_points.header.stamp = this->get_clock()->now();

    for (const auto& cluster : clusters) {
        if (cluster.points.empty()) continue;

        geometry_msgs::msg::PoseStamped ps_human, ps_robot;
        ps_human.header.frame_id = source_frame;
        ps_human.header.stamp = rclcpp::Time(0);
        ps_human.pose.position = cluster.centroid;
        ps_human.pose.orientation.w = 1.0; // <----- check if needs to be constraint from here, or send Points instead of Poses ----->

        try {
            // transform the pose to the robot's planning frame
            ps_robot = tf_buffer_->transform(ps_human, planning_frame);

            // check for NaN or Infinity
            if (std::isfinite(ps_robot.pose.position.x) && 
                std::isfinite(ps_robot.pose.position.y) && 
                std::isfinite(ps_robot.pose.position.z)) 
            {
                response->clustered_points.poses.push_back(ps_robot.pose);
            } 
            else {
                RCLCPP_WARN(this->get_logger(), "Calculated centroid contains non-finite values. Skipping cluster.");
            }

        } catch (const tf2::TransformException & ex) {
            RCLCPP_ERROR(this->get_logger(), "Could not transform to planning frame: %s", ex.what());
            response->success = false;
            response->message = "Could not transform to planning frame: " + std::string(ex.what());
            return;
        }
    }

    response->success = !response->clustered_points.poses.empty();
    response->message = "Found " + std::to_string(response->clustered_points.poses.size()) + " points.";
}

}  // namespace kiro_handover_calculation

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<kiro_handover_calculation::VolumeClusteringService>();
    
    // to allow concurrent callbacks (service requests while also receiving volume updates)
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    
    rclcpp::shutdown();
    return 0;
}
