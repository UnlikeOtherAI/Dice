#pragma once
#include "rendering/renderer.h"
#include "animation/animation_controller.h"
#include "rendering/face_mapper.h"
#include "geometry/polyhedra.h"
#include "geometry/chamfer.h"
#include "geometry/mesh_builder.h"
#include <memory>
#include <map>
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
    explicit DiceScene(int backend = 0);  // backend: 0=DEFAULT, used by platform adapters
    ~DiceScene() = default;

    void attachSurface(void* nativeWindow, uint32_t w, uint32_t h);
    void detachSurface();
    void resize(uint32_t w, uint32_t h);

    // NOTE: Synchronous mesh build. Avoid calling on the main thread for d32.
    uint32_t addDie(const DieConfig& config);
    void removeDie(uint32_t handle);

    // Roll: spin for `duration` seconds and land on face `result`
    void roll(uint32_t handle, int result, float duration);

    // Roll multiple dice simultaneously (all land at the same time)
    void rollAll(const std::vector<std::pair<uint32_t, int>>& rolls, float duration);

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
    std::map<uint32_t, Die> _dice;
    uint32_t _nextHandle = 1;

    void layoutDice();  // reposition dice when count changes
};

} // namespace dice3d
