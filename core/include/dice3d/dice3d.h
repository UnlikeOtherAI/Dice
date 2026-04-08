#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to a DiceScene
typedef struct Dice3DScene* Dice3DSceneRef;

Dice3DSceneRef dice3d_create(void);
void           dice3d_destroy(Dice3DSceneRef scene);

void dice3d_attach_surface(Dice3DSceneRef scene, void* nativeWindow,
                           uint32_t width, uint32_t height);
void dice3d_detach_surface(Dice3DSceneRef scene);
void dice3d_resize(Dice3DSceneRef scene, uint32_t width, uint32_t height);

uint32_t dice3d_add_die(Dice3DSceneRef scene, int sides,
                        float bevel,
                        float r, float g, float b, float a,
                        int whiteNumbers);
void dice3d_remove_die(Dice3DSceneRef scene, uint32_t handle);

void dice3d_roll(Dice3DSceneRef scene, uint32_t handle,
                 int result, float duration);

void dice3d_tick(Dice3DSceneRef scene, float dt);
void dice3d_render_frame(Dice3DSceneRef scene);

#ifdef __cplusplus
}
#endif
