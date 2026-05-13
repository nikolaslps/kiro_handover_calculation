#include "kiro_handover_calculation/image_relay.hpp"

namespace kiro_handover_calculation
{

ImageRelay::ImageRelay() : Node("image_relay_node"), activate_calc_(false) 
{
    // Declare parameters with defaults
    this->declare_parameter<bool>("use_depth", true);
    this->declare_parameter<bool>("image_compressed", true);

    use_depth_ = this->get_parameter("use_depth").as_bool();
    use_compressed_ = this->get_parameter("image_compressed").as_bool();

    this->declare_parameter<std::string>("color_img_topic", "/camera/camera/color/image_raw/compressed");
    this->declare_parameter<std::string>("color_info_topic", "/camera/camera/color/camera_info");
    this->declare_parameter<std::string>("depth_img_topic", "/camera/camera/aligned_depth_to_color/image_raw/compressedDepth");
    this->declare_parameter<std::string>("depth_info_topic", "/camera/camera/aligned_depth_to_color/camera_info");

    // Get parameter values
    color_img_topic_ = this->get_parameter("color_img_topic").as_string();
    color_info_topic_ = this->get_parameter("color_info_topic").as_string();
    depth_img_topic_ = this->get_parameter("depth_img_topic").as_string();
    depth_info_topic_ = this->get_parameter("depth_info_topic").as_string();

    // Status Subscriber
    sub_status_ = this->create_subscription<std_msgs::msg::Bool>(
        "/handover_status", 10, [this](const std_msgs::msg::Bool::SharedPtr msg) {
            this->activate_calc_ = msg->data;
        });

    // ROS4HRI camera topics based on the launch arguments and parameters
    // Color Image Relay Setup
    pub_c_info_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("/camera_info", 10);
    sub_c_info_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        color_info_topic_, 10, [this](const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
            if (activate_calc_) pub_c_info_->publish(*msg);
        });

    if (use_compressed_) {
        pub_c_img_comp_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/image/compressed", 10);
        sub_c_img_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
            color_img_topic_, 10, [this](const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
                if (activate_calc_) pub_c_img_comp_->publish(*msg);
            });
    } else {
        pub_c_img_raw_ = this->create_publisher<sensor_msgs::msg::Image>("/image", 10);
        sub_c_img_ = this->create_subscription<sensor_msgs::msg::Image>(
            color_img_topic_, 10, [this](const sensor_msgs::msg::Image::SharedPtr msg) {
                if (activate_calc_) pub_c_img_raw_->publish(*msg);
            });
    }

    // Depth Image Relay Setup
    if (use_depth_) {
        pub_d_info_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("/depth_info", 10);
        sub_d_info_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
            depth_info_topic_, 10, [this](const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
                if (activate_calc_) pub_d_info_->publish(*msg);
            });

        if (use_compressed_) {
            pub_d_img_comp_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/depth_image/compressed", 10);
            sub_d_img_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
                depth_img_topic_, 10, [this](const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
                    if (activate_calc_) pub_d_img_comp_->publish(*msg);
                });
        } else {
            pub_d_img_raw_ = this->create_publisher<sensor_msgs::msg::Image>("/depth_image", 10);
            sub_d_img_ = this->create_subscription<sensor_msgs::msg::Image>(
                depth_img_topic_, 10, [this](const sensor_msgs::msg::Image::SharedPtr msg) {
                    if (activate_calc_) pub_d_img_raw_->publish(*msg);
                });
        }
    }

    RCLCPP_INFO(this->get_logger(), "Image Relay Node initialized (Compressed: %s, Depth: %s)", 
                use_compressed_ ? "TRUE" : "FALSE", use_depth_ ? "TRUE" : "FALSE");
}

}  // namespace kiro_handover_calculation

int main(int argc, char * argv[]) 
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<kiro_handover_calculation::ImageRelay>());
    rclcpp::shutdown();
    return 0;
}