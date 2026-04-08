#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <random>

namespace dice3d {

class AnimationController {
public:
    enum class State { Idle, Spinning, Settled };

    AnimationController();

    // Begin a roll: spin for `duration` seconds and land on `target` orientation.
    void roll(const glm::quat& target, float duration);

    // Advance animation by dt seconds. Call every frame.
    void tick(float dt);

    State state() const { return _state; }
    glm::quat currentOrientation() const { return _orientation; }

private:
    State _state = State::Idle;
    glm::quat _orientation{1,0,0,0};

    glm::quat _target{1,0,0,0};
    float _duration = 0.0f;
    float _elapsed  = 0.0f;

    // Tumble: angular velocity (radians/sec, multi-axis)
    glm::vec3 _angularVelocity{0,0,0};

    // Correction quaternion: rotates the raw-end orientation into target
    // Blended in during the final 30% of the animation
    glm::quat _qRawEnd{1,0,0,0};
    glm::quat _qCorr{1,0,0,0};

    glm::quat _orientationAtCorrectionStart{1,0,0,0};
    bool _correctionStarted = false;

    std::mt19937 _rng;

    void initTumble();
};

} // namespace dice3d
