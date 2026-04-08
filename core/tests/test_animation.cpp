#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/animation/quaternion_utils.h"
#include "dice3d/animation/animation_controller.h"
#include <cmath>
using namespace dice3d;

TEST_CASE("slerp at t=0 returns qA") {
    glm::quat a(1,0,0,0), b(0,1,0,0);
    auto result = QuaternionUtils::slerp(a, b, 0.0f);
    REQUIRE_THAT(result.w, Catch::Matchers::WithinAbs(1.0f, 0.001f));
}

TEST_CASE("slerp at t=1 returns qB") {
    glm::quat a(1,0,0,0), b = glm::normalize(glm::quat(0,1,0,0));
    auto result = QuaternionUtils::slerp(a, b, 1.0f);
    REQUIRE_THAT(std::abs(result.w), Catch::Matchers::WithinAbs(std::abs(b.w), 0.001f));
}

TEST_CASE("slerp flips qB if dot < 0 (shortest path)") {
    glm::quat a(1,0,0,0);
    glm::quat b(-1,0,0,0);  // same rotation, opposite hemisphere
    auto result = QuaternionUtils::slerp(a, b, 0.5f);
    REQUIRE_THAT(result.w, Catch::Matchers::WithinAbs(1.0f, 0.01f));
}

TEST_CASE("easeOutQuint at t=0 is 0, t=1 is 1") {
    REQUIRE_THAT(QuaternionUtils::easeOutQuint(0.0f), Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(QuaternionUtils::easeOutQuint(1.0f), Catch::Matchers::WithinAbs(1.0f, 0.001f));
}

TEST_CASE("animation starts in idle state") {
    AnimationController ctrl;
    REQUIRE(ctrl.state() == AnimationController::State::Idle);
}

TEST_CASE("after roll(), state is Spinning") {
    AnimationController ctrl;
    glm::quat target(1,0,0,0);
    ctrl.roll(target, 2.0f);
    REQUIRE(ctrl.state() == AnimationController::State::Spinning);
}

TEST_CASE("after full duration, state is Settled") {
    AnimationController ctrl;
    glm::quat target = glm::normalize(glm::quat(0.5f, 0.5f, 0.5f, 0.5f));
    ctrl.roll(target, 1.0f);
    ctrl.tick(0.5f);
    REQUIRE(ctrl.state() == AnimationController::State::Spinning);
    ctrl.tick(0.6f);  // total > 1.0s
    REQUIRE(ctrl.state() == AnimationController::State::Settled);
}

TEST_CASE("settled orientation matches target") {
    AnimationController ctrl;
    glm::quat target = glm::angleAxis(1.23f, glm::normalize(glm::vec3(1,1,0)));
    ctrl.roll(target, 0.5f);
    ctrl.tick(1.0f);  // advance well past duration
    glm::quat result = ctrl.currentOrientation();
    float d = std::abs(glm::dot(result, target));
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(1.0f, 0.001f));
}
