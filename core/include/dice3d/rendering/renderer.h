#pragma once
#include <cstdint>
#include "dice3d/geometry/mesh_builder.h"

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
#include <filament/LightManager.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
    filament::Material*  _material  = nullptr;
    utils::Entity        _sunLight;

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
    void attachSurface(void*, uint32_t, uint32_t) {}
    void detachSurface() {}
    void resize(uint32_t, uint32_t) {}
    uint32_t addDie(const GpuMesh&, const glm::vec4&, bool) { return 0; }
    void removeDie(uint32_t) {}
    void setDieTransform(uint32_t, const glm::quat&, const glm::vec3&) {}
    void renderFrame() {}
};

#endif // DICE3D_HAVE_FILAMENT

} // namespace dice3d
