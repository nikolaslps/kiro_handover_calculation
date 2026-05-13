#pragma once

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/bool.hpp"

#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Vector3.h>

namespace kiro_handover_calculation
{

class ImageRelay : public rclcpp::Node 
{
public:
    ImageRelay();

private:
    // Status Subscription
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_status_;

    // Camera Info (Always same type)
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr sub_c_info_;
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr sub_d_info_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr pub_c_info_;
    rclcpp::Publisher<sensor_msgs::msg::CameraInfo>::SharedPtr pub_d_info_;

    // Image Subscriptions
    rclcpp::SubscriptionBase::SharedPtr sub_c_img_;
    rclcpp::SubscriptionBase::SharedPtr sub_d_img_;

    // Compressed Image Publishers
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr pub_c_img_comp_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr pub_d_img_comp_;

    // Raw Image Publishers
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_c_img_raw_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_d_img_raw_;
    
    bool activate_calc_;
    bool use_compressed_;
    bool use_depth_;

    // Parameters
    std::string color_img_topic_;
    std::string color_info_topic_;
    std::string depth_img_topic_;
    std::string depth_info_topic_;
};

}  // namespace kiro_handover_calculation