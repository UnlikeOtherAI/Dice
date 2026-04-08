#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace dice3d {

class QuaternionUtils {
public:
    // Shortest-path slerp (flips qB if dot(qA,qB) < 0)
    static glm::quat slerp(const glm::quat& a, const glm::quat& b, float t);

    // Easing functions — return value in [0,1] for t in [0,1]
    static float easeOutCubic(float t);
    static float easeOutQuint(float t);   // recommended for floating dice
    static float easeOutExpo(float t);
    static float easeInCubic(float t);    // used for correction blend
};

} // namespace dice3d
