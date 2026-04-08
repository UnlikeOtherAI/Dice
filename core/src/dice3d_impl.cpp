#include "dice3d/dice3d.h"
#include "dice3d/dice_scene.h"
#include <stdexcept>

// Map the opaque C pointer to a DiceScene.
// We allocate DiceScene on the heap and cast to/from void*.
static dice3d::DiceScene* toScene(Dice3DSceneRef ref) {
    return static_cast<dice3d::DiceScene*>(static_cast<void*>(ref));
}

extern "C" {

Dice3DSceneRef dice3d_create(void) {
    return static_cast<Dice3DSceneRef>(static_cast<void*>(new dice3d::DiceScene()));
}

void dice3d_destroy(Dice3DSceneRef scene) {
    delete toScene(scene);
}

void dice3d_attach_surface(Dice3DSceneRef scene, void* nativeWindow,
                           uint32_t width, uint32_t height) {
    toScene(scene)->attachSurface(nativeWindow, width, height);
}

void dice3d_detach_surface(Dice3DSceneRef scene) {
    toScene(scene)->detachSurface();
}

void dice3d_resize(Dice3DSceneRef scene, uint32_t width, uint32_t height) {
    toScene(scene)->resize(width, height);
}

void dice3d_set_camera_distance(Dice3DSceneRef scene, float distance) {
    toScene(scene)->setCameraDistance(distance);
}

uint32_t dice3d_add_die(Dice3DSceneRef scene, int sides,
                        float bevel,
                        float r, float g, float b, float a,
                        int whiteNumbers) {
    dice3d::DieConfig cfg;
    cfg.sides        = sides;
    cfg.bevelFactor  = bevel;
    cfg.dieColor     = {r, g, b, a};
    cfg.whiteNumbers = (whiteNumbers != 0);
    return toScene(scene)->addDie(cfg);
}

void dice3d_remove_die(Dice3DSceneRef scene, uint32_t handle) {
    toScene(scene)->removeDie(handle);
}

void dice3d_roll(Dice3DSceneRef scene, uint32_t handle,
                 int result, float duration) {
    try {
        toScene(scene)->roll(handle, result, duration);
    } catch (const std::out_of_range&) {
        // Invalid face number — silently ignore; die stays in current orientation
    }
}

void dice3d_set_presentation_mode(Dice3DSceneRef scene, uint32_t handle,
                                  Dice3DPresentationMode mode,
                                  float speed, float duration) {
    toScene(scene)->setPresentationMode(
        handle,
        static_cast<dice3d::PresentationMode>(mode),
        speed,
        duration
    );
}

void dice3d_set_idle_spin_speed(Dice3DSceneRef scene, uint32_t handle, float speed) {
    toScene(scene)->setIdleSpinSpeed(handle, speed);
}

void dice3d_begin_drag(Dice3DSceneRef scene, uint32_t handle) {
    toScene(scene)->beginDrag(handle);
}

void dice3d_drag_by(Dice3DSceneRef scene, uint32_t handle, float deltaX, float deltaY) {
    toScene(scene)->dragBy(handle, deltaX, deltaY);
}

void dice3d_end_drag(Dice3DSceneRef scene, uint32_t handle) {
    toScene(scene)->endDrag(handle);
}

void dice3d_tick(Dice3DSceneRef scene, float dt) {
    toScene(scene)->tick(dt);
}

void dice3d_render_frame(Dice3DSceneRef scene) {
    toScene(scene)->renderFrame();
}

void dice3d_load_material(Dice3DSceneRef scene, const void* data, size_t size) {
    toScene(scene)->loadMaterial(data, size);
}

void dice3d_load_atlas(Dice3DSceneRef scene, const void* rgbaData,
                       uint32_t width, uint32_t height) {
    toScene(scene)->loadAtlas(rgbaData, width, height);
}

} // extern "C"
