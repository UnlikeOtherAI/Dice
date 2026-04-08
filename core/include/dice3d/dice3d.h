#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to a DiceScene
typedef struct Dice3DScene* Dice3DSceneRef;

typedef enum Dice3DPresentationMode {
    DICE3D_PRESENTATION_STATIC = 0,
    DICE3D_PRESENTATION_SPIN_IN = 1,
    DICE3D_PRESENTATION_IDLE_SPIN = 2,
} Dice3DPresentationMode;

Dice3DSceneRef dice3d_create(void);
void           dice3d_destroy(Dice3DSceneRef scene);

void dice3d_attach_surface(Dice3DSceneRef scene, void* nativeWindow,
                           uint32_t width, uint32_t height);
void dice3d_detach_surface(Dice3DSceneRef scene);
void dice3d_resize(Dice3DSceneRef scene, uint32_t width, uint32_t height);
void dice3d_set_camera_distance(Dice3DSceneRef scene, float distance);

// NOTE: Synchronous — builds mesh on calling thread. Do not call on the main/render thread
// for large die types (d32). Returns 0 on failure.
uint32_t dice3d_add_die(Dice3DSceneRef scene, int sides,
                        float bevel,
                        float r, float g, float b, float a,
                        int whiteNumbers);
void dice3d_remove_die(Dice3DSceneRef scene, uint32_t handle);

void dice3d_roll(Dice3DSceneRef scene, uint32_t handle,
                 int result, float duration);
void dice3d_set_presentation_mode(Dice3DSceneRef scene, uint32_t handle,
                                  Dice3DPresentationMode mode,
                                  float speed, float duration);
void dice3d_set_idle_spin_speed(Dice3DSceneRef scene, uint32_t handle, float speed);
void dice3d_begin_drag(Dice3DSceneRef scene, uint32_t handle);
void dice3d_drag_by(Dice3DSceneRef scene, uint32_t handle, float deltaX, float deltaY);
void dice3d_end_drag(Dice3DSceneRef scene, uint32_t handle);

void dice3d_tick(Dice3DSceneRef scene, float dt);
void dice3d_render_frame(Dice3DSceneRef scene);

// Optional: call before addDie to enable PBR number rendering.
// data = compiled .filamat binary, size = byte length.
void dice3d_load_material(Dice3DSceneRef scene, const void* data, size_t size);
// Load the atlas texture for the current die type (raw RGBA8 pixels, width*height*4 bytes).
// Call after dice3d_load_material, before dice3d_add_die, once per die type.
void dice3d_load_atlas(Dice3DSceneRef scene, const void* rgbaData, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
