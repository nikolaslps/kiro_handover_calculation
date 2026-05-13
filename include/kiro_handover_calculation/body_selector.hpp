#pragma once

#include <tf2_ros/buffer.h>
#include <set>
#include <string>
#include <memory>

namespace kiro_handover_calculation
{
class BodySelector
{
public:
    BodySelector(std::shared_ptr<tf2_ros::Buffer> tf_buffer, std::string camera_frame);

    void setCameraFrame(std::string camera_frame);
    void setTrackedBodies(const std::set<std::string>& ids);

    void setTfTimeout(double seconds);
    void setMaxTfAge(double seconds);

    void setBodyPrefix(std::string prefix);  // default "body_"
    void setHeadPrefix(std::string prefix);  // default "head_"

    std::string select(const std::string& current_active_id);

private:
    bool hasValidBodyTf(const std::string& body_id) const;
    bool hasHeadConnection(const std::string& body_id) const;
    bool tryDistanceToCamera(const std::string& body_id, double& out_dist_m) const;

private:
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::string camera_frame_;
    std::set<std::string> tracked_;

    std::string body_prefix_{"body_"};
    std::string head_prefix_{"head_"};

    double tf_timeout_s_{0.05}; // minimum time to wait for TF
    double max_tf_age_s_{0.35}; // maximum age of TF to consider valid
};
}  // namespace kiro_handover_calculation