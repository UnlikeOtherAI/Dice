#include "dice3d/animation/quaternion_utils.h"
#include <cmath>
using namespace dice3d;

glm::quat QuaternionUtils::slerp(const glm::quat& a, const glm::quat& b, float t) {
    glm::quat bb = b;
    if (glm::dot(a, b) < 0.0f) bb = -bb;  // shortest path
    return glm::normalize(glm::slerp(a, bb, t));
}

float QuaternionUtils::easeOutCubic(float t) { float x = 1-t; return 1 - x*x*x; }
float QuaternionUtils::easeOutQuint(float t) { float x = 1-t; return 1 - x*x*x*x*x; }
float QuaternionUtils::easeOutExpo(float t)  { return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f*t); }
float QuaternionUtils::easeInCubic(float t)  { return t*t*t; }
