#pragma once

#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Vector3.h>

namespace kiro_handover_calculation
{

tf2::Vector3 computeArmFK(const tf2::Vector3& shoulder_pos,
                          const tf2::Vector3& shoulder_axis,
                          double shoulder_angle,
                          double elbow_angle,
                          double upper_arm_length,
                          double forearm_length);

}  // namespace kiro_handover_calculation