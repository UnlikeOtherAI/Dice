#pragma once
#include "rendering/renderer.h"
#include "animation/animation_controller.h"
#include "rendering/face_mapper.h"
#include "geometry/polyhedra.h"
#include "geometry/chamfer.h"
#include "geometry/mesh_builder.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

namespace dice3d {

struct DieConfig {
    int   sides;
    float bevelFactor = 0.05f;
    glm::vec4 dieColor{0.8f, 0.1f, 0.1f, 1.0f};  // RGBA
    bool  whiteNumbers = true;
};

class DiceScene {
public:
    explicit DiceScene();
    ~DiceScene() = default;

    void attachSurface(void* nativeWindow, uint32_t w, uint32_t h);
    void detachSurface();
    void resize(uint32_t w, uint32_t h);

    // Returns opaque handle for the die
    uint32_t addDie(const DieConfig& config);
    void removeDie(uint32_t handle);

    // Roll: spin for `duration` seconds and land on face `result`
    void roll(uint32_t handle, int result, float duration);

    // Call every frame (from display link / Choreographer callback)
    void tick(float dt);
    void renderFrame();

private:
    struct Die {
        uint32_t renderHandle;
        std::unique_ptr<AnimationController> anim;
        std::unique_ptr<FaceMapper> faceMap;
        glm::vec3 position{0,0,0};
    };

    std::unique_ptr<Renderer> _renderer;
    std::unordered_map<uint32_t, Die> _dice;
    uint32_t _nextHandle = 1;

    void layoutDice();  // reposition dice when count changes
};

} // namespace dice3d
