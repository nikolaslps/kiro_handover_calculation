#include "kiro_handover_calculation/body_selector.hpp"

#include <tf2/exceptions.h>
#include <tf2/time.h>

#include <cmath>
#include <limits>

namespace kiro_handover_calculation
{

BodySelector::BodySelector(std::shared_ptr<tf2_ros::Buffer> tf_buffer, std::string camera_frame)
: tf_buffer_(std::move(tf_buffer)), camera_frame_(std::move(camera_frame))
{
}

void BodySelector::setCameraFrame(std::string camera_frame) 
{ 
    camera_frame_ = std::move(camera_frame); 
}

void BodySelector::setTrackedBodies(const std::set<std::string>& ids) 
{ 
    tracked_ = ids; 
}

void BodySelector::setTfTimeout(double seconds) 
{ 
    tf_timeout_s_ = seconds; 
}

void BodySelector::setMaxTfAge(double seconds) 
{ 
    max_tf_age_s_ = seconds; 
}

void BodySelector::setBodyPrefix(std::string prefix) 
{ 
    body_prefix_ = std::move(prefix); 
}

void BodySelector::setHeadPrefix(std::string prefix) 
{ 
    head_prefix_ = std::move(prefix); 
}

bool BodySelector::hasValidBodyTf(const std::string& body_id) const
{
    const std::string body_frame = body_prefix_ + body_id;

    if (!tf_buffer_->canTransform(camera_frame_, body_frame, tf2::TimePointZero,
                                    tf2::durationFromSec(tf_timeout_s_))) {
        return false;
    }

    if (max_tf_age_s_ > 0.0) {
        try {
            const auto tr = tf_buffer_->lookupTransform(camera_frame_, body_frame, tf2::TimePointZero);

            const double age_s = (rclcpp::Clock(RCL_ROS_TIME).now() - tr.header.stamp).seconds();
            if (!std::isfinite(age_s) || age_s > max_tf_age_s_) 
            {
                return false;
            }
        } catch (const tf2::TransformException&) {
            return false;
        }
    }

    return true;
}

bool BodySelector::hasHeadConnection(const std::string& body_id) const
{
    const std::string body_frame = body_prefix_ + body_id;
    const std::string head_frame = head_prefix_ + body_id;

    // If head TF exists and is connected, this returns true
    return tf_buffer_->canTransform(body_frame, head_frame, tf2::TimePointZero,
                                    tf2::durationFromSec(tf_timeout_s_));
}

bool BodySelector::tryDistanceToCamera(const std::string& body_id, double& out_dist_m) const
{
    const std::string body_frame = body_prefix_ + body_id;

    if (!tf_buffer_->canTransform(camera_frame_, body_frame, tf2::TimePointZero,
                                    tf2::durationFromSec(tf_timeout_s_))) {
        return false;
    }

    try {
        const auto tr = tf_buffer_->lookupTransform(camera_frame_, body_frame, tf2::TimePointZero);
        const double x = tr.transform.translation.x;
        const double y = tr.transform.translation.y;
        const double z = tr.transform.translation.z;
        out_dist_m = std::sqrt(x * x + y * y + z * z);
        return std::isfinite(out_dist_m);
    } catch (const tf2::TransformException&) {
        return false;
    }
}

std::string BodySelector::select(const std::string& current_active_id)
{
    if (tracked_.empty()) return "";

    if (tracked_.size() == 1) 
    {
        return *tracked_.begin();
    }

    // Keep current only if tracked AND body TF is valid AND body->head is connected
    if (!current_active_id.empty() && tracked_.count(current_active_id) > 0 &&
        hasValidBodyTf(current_active_id) && hasHeadConnection(current_active_id)) 
    {
        return current_active_id;
    }

    // Prefer candidates that have head connection. Among them pick closest
    std::string closest_id;
    double min_distance = std::numeric_limits<double>::infinity();

    for (const auto& id : tracked_) {
        if (!hasHeadConnection(id)) continue;

        double d;
        if (!tryDistanceToCamera(id, d)) continue;

        if (d < min_distance) {
            min_distance = d;
            closest_id = id;
        }
    }

    // If nobody has head TF, fall back to "closest with body TF"
    if (closest_id.empty()) {
        for (const auto& id : tracked_) {
            double d;
            if (!tryDistanceToCamera(id, d)) continue;

            if (d < min_distance) {
                min_distance = d;
                closest_id = id;
            }
        }
    }

    // If still nothing (no TF at all), pick first tracked id
    if (closest_id.empty()) 
    {
        closest_id = *tracked_.begin();
    }
    return closest_id;
}

}  // namespace kiro_handover_calculation