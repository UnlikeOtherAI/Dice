#define GLM_ENABLE_EXPERIMENTAL
#include "dice3d/animation/animation_controller.h"
#include "dice3d/animation/quaternion_utils.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
using namespace dice3d;

// Correction blend starts at this fraction of duration
static constexpr float kCorrectionStart = 0.70f;
static constexpr float kPresentationSlowdown = 0.15f;

AnimationController::AnimationController()
    : _rng(std::random_device{}()) {}

void AnimationController::startIdleSpin(float speed) {
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    glm::vec3 axis(axisDist(_rng), axisDist(_rng), axisDist(_rng));
    if (glm::length(axis) < 1e-6f) axis = glm::vec3(0, 1, 0);
    _idleAngularVelocity = glm::normalize(axis) * std::max(speed, 0.0f);
    _state = State::IdleSpinning;
}

void AnimationController::stopIdleSpin() {
    _idleAngularVelocity = glm::vec3(0.0f);
    _state = State::Idle;
}

void AnimationController::startPresentationSpin(float speed, float duration) {
    startIdleSpin(speed);
    _presentationDuration = std::max(duration, 0.0f);
    _presentationElapsed = 0.0f;
}

void AnimationController::beginDrag() {
    _state = State::Idle;
}

void AnimationController::dragBy(float deltaYaw, float deltaPitch) {
    glm::quat yaw = glm::angleAxis(deltaYaw, glm::vec3(0, 1, 0));
    glm::quat pitch = glm::angleAxis(deltaPitch, glm::vec3(1, 0, 0));
    _orientation = glm::normalize(yaw * pitch * _orientation);
}

void AnimationController::endDrag() {}

void AnimationController::roll(const glm::quat& target, float duration) {
    _target   = glm::normalize(target);
    _duration = std::max(duration, 0.001f);  // guard: zero duration
    _elapsed  = 0.0f;
    _state    = State::Spinning;
    _correctionStarted = false;
    initTumble();
}

void AnimationController::initTumble() {
    // Random multi-axis angular velocity: 8-20 rad/s total speed
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speedDist(8.0f, 20.0f);
    glm::vec3 axis(axisDist(_rng), axisDist(_rng), axisDist(_rng));
    if (glm::length(axis) < 1e-6f) axis = glm::vec3(1,0,0);  // degenerate fallback
    _angularVelocity = glm::normalize(axis) * speedDist(_rng);

    // Numerically simulate where the tumble ends after _duration
    // using the same integration as tick() — 60 steps at uniform dt
    glm::quat q = _orientation;
    float dt_sim = _duration / 60.0f;
    for (int i = 0; i < 60; i++) {
        float t = (float)i / 60.0f;
        float speedMul = 1.0f - QuaternionUtils::easeOutQuint(t);
        glm::vec3 w = _angularVelocity * speedMul;
        float angle = glm::length(w) * dt_sim;
        if (angle > 1e-6f) {
            glm::quat delta = glm::angleAxis(angle, glm::normalize(w));
            q = glm::normalize(delta * q);
        }
    }
    _qRawEnd = q;
    // Correction: the quaternion that rotates qRawEnd into _target
    _qCorr = glm::normalize(glm::inverse(_qRawEnd) * _target);
}

void AnimationController::tick(float dt) {
    if (_state == State::IdleSpinning) {
        if (_presentationDuration > 0.0f) {
            _presentationElapsed += dt;
            float t = std::min(_presentationElapsed / _presentationDuration, 1.0f);
            float speedMul = 1.0f - (1.0f - kPresentationSlowdown) * t;
            float angle = glm::length(_idleAngularVelocity) * speedMul * dt;
            if (angle > 1e-6f) {
                glm::quat delta = glm::angleAxis(angle, glm::normalize(_idleAngularVelocity));
                _orientation = glm::normalize(delta * _orientation);
            }
            if (t >= 1.0f) {
                stopIdleSpin();
                _state = State::Settled;
            }
            return;
        }
        float angle = glm::length(_idleAngularVelocity) * dt;
        if (angle > 1e-6f) {
            glm::quat delta = glm::angleAxis(angle, glm::normalize(_idleAngularVelocity));
            _orientation = glm::normalize(delta * _orientation);
        }
        return;
    }
    if (_state != State::Spinning) return;

    _elapsed += dt;
    float t = std::min(_elapsed / _duration, 1.0f);

    // Phase 1: integrate angular velocity (tumble with easeOutQuint decay)
    float speedMul = 1.0f - QuaternionUtils::easeOutQuint(t);
    glm::vec3 w = _angularVelocity * speedMul;
    float angle = glm::length(w) * dt;
    if (angle > 1e-6f) {
        glm::quat delta = glm::angleAxis(angle, glm::normalize(w));
        _orientation = glm::normalize(delta * _orientation);
    }

    // Phase 2: blend correction in final 30% — slerp absolutely from snapshot to target
    if (t > kCorrectionStart) {
        if (!_correctionStarted) {
            _orientationAtCorrectionStart = _orientation;
            _correctionStarted = true;
        }
        float corrT = (t - kCorrectionStart) / (1.0f - kCorrectionStart);
        float blend = QuaternionUtils::easeInCubic(corrT);
        _orientation = QuaternionUtils::slerp(
            _orientationAtCorrectionStart, _target, blend
        );
    }

    if (t >= 1.0f) {
        _orientation = _target;
        _state = State::Settled;
    }
}
