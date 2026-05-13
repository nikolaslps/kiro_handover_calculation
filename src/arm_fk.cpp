#include "kiro_handover_calculation/arm_fk.hpp"

#include <cmath>

namespace kiro_handover_calculation
{

tf2::Vector3 computeArmFK(const tf2::Vector3& shoulder_pos,
                          const tf2::Vector3& shoulder_axis,
                          double shoulder_angle,
                          double elbow_angle,
                          double upper_arm_length,
                          double forearm_length)
{
    tf2::Matrix3x3 A0_1_rot;
    tf2::Vector3 A0_1_trans;

    if (shoulder_axis == tf2::Vector3(0, 1, 0)) {
        A0_1_rot = tf2::Matrix3x3(
            std::cos(shoulder_angle),  0, std::sin(shoulder_angle),
                    0,                 1,           0,
            -std::sin(shoulder_angle), 0, std::cos(shoulder_angle)
        );
        A0_1_trans = shoulder_pos + A0_1_rot * tf2::Vector3(upper_arm_length, 0, 0);
    } else {
        return tf2::Vector3(0, 0, 0);
    }

    tf2::Matrix3x3 A1_E_rot;
    tf2::Vector3 A1_E_trans;

    if (shoulder_axis == tf2::Vector3(0, 1, 0)) {
        A1_E_rot = tf2::Matrix3x3(
            std::cos(elbow_angle),  0, std::sin(elbow_angle),
                    0,              1,          0,
            -std::sin(elbow_angle), 0, std::cos(elbow_angle)
        );
        A1_E_trans = A1_E_rot * tf2::Vector3(forearm_length, 0, 0);
    } else {
        return tf2::Vector3(0, 0, 0);
    }

    return A0_1_trans + A0_1_rot * A1_E_trans; // A0_E_trans
}

}  // namespace kiro_handover_calculation