#pragma once 

#include <rclcpp/rclcpp.hpp>

#include <visualization_msgs/msg/marker_array.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include "kiro_handover_interfaces/srv/cluster_handover_volume.hpp"

#include <vector>
#include <string>

namespace kiro_handover_calculation 
{

struct kMeansCluster {
    int cluster_id;
    geometry_msgs::msg::Point centroid;
    std::vector<geometry_msgs::msg::Point> points;
};

class VolumeClusteringService : public rclcpp::Node 
{
public:
    VolumeClusteringService();

private:
    void handleClusterRequest(const std::shared_ptr<kiro_handover_interfaces::srv::ClusterHandoverVolume::Request> request,
                                std::shared_ptr<kiro_handover_interfaces::srv::ClusterHandoverVolume::Response> response);
    
    std::vector<kMeansCluster> kMeansClustering(const std::vector<geometry_msgs::msg::Point>& points, 
                                                int k, 
                                                int max_iterations = 100);

    std::string planning_frame_;

    // TF2
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    // Multithreading callback group to allow service and subscription to run in parallel
    rclcpp::CallbackGroup::SharedPtr callback_group_;

    // Service
    rclcpp::Service<kiro_handover_interfaces::srv::ClusterHandoverVolume>::SharedPtr service_;
};

}  // namespace kiro_handover_calculation
