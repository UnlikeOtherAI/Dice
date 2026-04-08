#pragma once
#include <cstdint>
#include "dice3d/geometry/mesh_builder.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef DICE3D_HAVE_FILAMENT
#include <filament/Engine.h>
#include <filament/SwapChain.h>
#include <filament/Renderer.h>
#include <filament/View.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <unordered_map>
#endif

namespace dice3d {

#ifdef DICE3D_HAVE_FILAMENT

struct DieInstance {
    uint32_t handle;
    utils::Entity entity;
    filament::VertexBuffer* vb   = nullptr;
    filament::IndexBuffer*  ib   = nullptr;
    filament::MaterialInstance* matInst = nullptr;
    glm::vec3 position{0,0,0};
};

class Renderer {
public:
    explicit Renderer(filament::Engine::Backend backend = filament::Engine::Backend::DEFAULT);
    ~Renderer();

    void attachSurface(void* nativeWindow, uint32_t width, uint32_t height);
    void detachSurface();
    void resize(uint32_t width, uint32_t height);
    void setCameraDistance(float distance);

    // Call before addDie. filamatData = compiled .filamat binary.
    void loadMaterial(const void* filamatData, size_t size);
    // Call before addDie. rgbaData = raw RGBA8 pixels (width*height*4 bytes).
    void loadAtlasTexture(const void* rgbaData, uint32_t width, uint32_t height);

    uint32_t addDie(const GpuMesh& mesh, const glm::vec4& dieColor, bool whiteNumbers);
    void removeDie(uint32_t handle);
    void setDieTransform(uint32_t handle, const glm::quat& orientation, const glm::vec3& position);

    void renderFrame();

    filament::Engine* engine() { return _engine; }

private:
    filament::Engine*    _engine    = nullptr;
    filament::SwapChain* _swapChain = nullptr;
    filament::Renderer*  _renderer  = nullptr;
    filament::View*      _view      = nullptr;
    filament::Scene*     _scene     = nullptr;
    filament::Camera*    _camera    = nullptr;
    utils::Entity        _cameraEntity;
    filament::Material*  _material      = nullptr;
    filament::Texture*   _atlasTexture  = nullptr;
    filament::IndirectLight* _ibl       = nullptr;
    utils::Entity        _sunLight;
    uint32_t             _viewportWidth  = 1;
    uint32_t             _viewportHeight = 1;
    float                _cameraDistance = 15.0f;

    std::unordered_map<uint32_t, DieInstance> _dice;
    uint32_t _nextHandle = 1;

    void setupCamera(uint32_t width, uint32_t height);
    void setupLighting();
    void destroyDieResources(DieInstance& die);
};

#else // stub for non-Filament builds

class Renderer {
public:
    explicit Renderer(int backend = 0) {}
    ~Renderer() = default;
    void loadMaterial(const void*, size_t) {}
    void loadAtlasTexture(const void*, uint32_t, uint32_t) {}
    void attachSurface(void*, uint32_t, uint32_t) {}
    void detachSurface() {}
    void resize(uint32_t, uint32_t) {}
    void setCameraDistance(float) {}
    uint32_t addDie(const GpuMesh&, const glm::vec4&, bool) { return 0; }
    void removeDie(uint32_t) {}
    void setDieTransform(uint32_t, const glm::quat&, const glm::vec3&) {}
    void renderFrame() {}
};

#endif // DICE3D_HAVE_FILAMENT

} // namespace dice3d
