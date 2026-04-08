#ifdef DICE3D_HAVE_FILAMENT

#define GLM_ENABLE_EXPERIMENTAL
#include "dice3d/rendering/renderer.h"
#include <filament/Camera.h>
#include <filament/Color.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <math/mat4.h>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
using namespace dice3d;

Renderer::Renderer(filament::Engine::Backend backend) {
    _engine   = filament::Engine::create(backend);
    _renderer = _engine->createRenderer();
    _view     = _engine->createView();
    _scene    = _engine->createScene();
    _camera   = nullptr;

    _view->setScene(_scene);
    _view->setBlendMode(filament::View::BlendMode::TRANSLUCENT);
    _view->setPostProcessingEnabled(false);  // no tone-mapping for transparent BG

    // Camera entity
    _cameraEntity = utils::EntityManager::get().create();
    _camera = _engine->createCamera(_cameraEntity);
    _view->setCamera(_camera);

    setupLighting();
}

Renderer::~Renderer() {
    // Destroy all die resources
    for (auto& [h, die] : _dice) destroyDieResources(die);
    _dice.clear();

    // Destroy light
    if (_sunLight) {
        _scene->remove(_sunLight);
        _engine->getLightManager().destroy(_sunLight);
        utils::EntityManager::get().destroy(_sunLight);
    }

    // Destroy material
    if (_material) _engine->destroy(_material);

    // Destroy view/scene/renderer
    _engine->destroyCameraComponent(_cameraEntity);
    utils::EntityManager::get().destroy(_cameraEntity);
    _engine->destroy(_view);
    _engine->destroy(_scene);
    _engine->destroy(_renderer);

    // Swap chain must be destroyed before engine
    if (_swapChain) _engine->destroy(_swapChain);

    filament::Engine::destroy(&_engine);
}

void Renderer::attachSurface(void* nativeWindow, uint32_t w, uint32_t h) {
    detachSurface();
    _swapChain = _engine->createSwapChain(
        nativeWindow,
        filament::SwapChain::CONFIG_TRANSPARENT
    );
    _view->setBlendMode(filament::View::BlendMode::TRANSLUCENT);
    resize(w, h);
}

void Renderer::detachSurface() {
    if (!_swapChain) return;
    _engine->flushAndWait();
    _engine->destroy(_swapChain);
    _swapChain = nullptr;
}

void Renderer::resize(uint32_t w, uint32_t h) {
    _view->setViewport({0, 0, w, h});
    setupCamera(w, h);
}

void Renderer::setupCamera(uint32_t w, uint32_t h) {
    float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
    // Perspective camera: 45 deg FOV, near=0.1, far=100
    _camera->setProjection(45.0, aspect, 0.1, 100.0);
    // Position camera 5 units back looking at origin
    _camera->lookAt({0,0,5}, {0,0,0}, {0,1,0});
}

void Renderer::setupLighting() {
    _sunLight = utils::EntityManager::get().create();
    filament::LightManager::Builder(filament::LightManager::Type::SUN)
        .color(filament::Color::toLinear<filament::ACCURATE>({1.0f, 1.0f, 1.0f}))
        .intensity(100000.0f)
        .direction({0.0f, -1.0f, -0.5f})
        .sunAngularRadius(1.9f)
        .castShadows(false)
        .build(*_engine, _sunLight);
    _scene->addEntity(_sunLight);
}

uint32_t Renderer::addDie(const GpuMesh& mesh, const glm::vec4& dieColor, bool /*whiteNumbers*/) {
    uint32_t handle = _nextHandle++;

    DieInstance die;
    die.handle = handle;

    // --- Vertex buffer ---
    auto vbBuilder = filament::VertexBuffer::Builder()
        .vertexCount((uint32_t)mesh.positions.size())
        .bufferCount(4)  // positions, tangents, uvs, colors
        .attribute(filament::VertexAttribute::POSITION, 0,
                   filament::VertexBuffer::AttributeType::FLOAT3)
        .attribute(filament::VertexAttribute::TANGENTS, 1,
                   filament::VertexBuffer::AttributeType::HALF4)
        .attribute(filament::VertexAttribute::UV0, 2,
                   filament::VertexBuffer::AttributeType::FLOAT2)
        .attribute(filament::VertexAttribute::COLOR, 3,
                   filament::VertexBuffer::AttributeType::FLOAT3)
        .normalized(filament::VertexAttribute::TANGENTS);

    die.vb = vbBuilder.build(*_engine);

    // Upload positions
    die.vb->setBufferAt(*_engine, 0,
        filament::VertexBuffer::BufferDescriptor(
            mesh.positions.data(),
            mesh.positions.size() * sizeof(glm::vec3)));

    // Convert tangent quaternions to HALF4 and upload
    std::vector<uint16_t> half4tangents;
    half4tangents.reserve(mesh.tangents.size() * 4);
    for (auto& t : mesh.tangents) {
        // Simple float-to-half conversion (sign + 5-bit exp + 10-bit mantissa)
        auto toHalf = [](float f) -> uint16_t {
            uint32_t x; memcpy(&x, &f, 4);
            uint16_t h = (x >> 16) & 0x8000;  // sign
            int e = ((x >> 23) & 0xFF) - 127 + 15;
            if (e <= 0) return h;
            if (e >= 31) return h | 0x7C00;
            return h | (e << 10) | ((x >> 13) & 0x3FF);
        };
        half4tangents.push_back(toHalf(t.x));
        half4tangents.push_back(toHalf(t.y));
        half4tangents.push_back(toHalf(t.z));
        half4tangents.push_back(toHalf(t.w));
    }
    die.vb->setBufferAt(*_engine, 1,
        filament::VertexBuffer::BufferDescriptor(
            half4tangents.data(),
            half4tangents.size() * sizeof(uint16_t)));

    // Upload UVs
    die.vb->setBufferAt(*_engine, 2,
        filament::VertexBuffer::BufferDescriptor(
            mesh.uvs.data(),
            mesh.uvs.size() * sizeof(glm::vec2)));

    // Upload per-vertex color (die color for all verts)
    std::vector<glm::vec3> colors(mesh.positions.size(),
                                  glm::vec3(dieColor.r, dieColor.g, dieColor.b));
    die.vb->setBufferAt(*_engine, 3,
        filament::VertexBuffer::BufferDescriptor(
            colors.data(),
            colors.size() * sizeof(glm::vec3)));

    // --- Index buffer ---
    die.ib = filament::IndexBuffer::Builder()
        .indexCount((uint32_t)mesh.indices.size())
        .bufferType(filament::IndexBuffer::IndexType::UINT)
        .build(*_engine);
    die.ib->setBuffer(*_engine,
        filament::IndexBuffer::BufferDescriptor(
            mesh.indices.data(),
            mesh.indices.size() * sizeof(uint32_t)));

    // --- Entity + renderable ---
    die.entity = utils::EntityManager::get().create();
    filament::RenderableManager::Builder(1)
        .boundingBox({{-2,-2,-2},{2,2,2}})
        .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES,
                  die.vb, die.ib)
        .culling(false)
        .receiveShadows(false)
        .castShadows(false)
        .build(*_engine, die.entity);

    _scene->addEntity(die.entity);
    _dice[handle] = std::move(die);
    return handle;
}

void Renderer::removeDie(uint32_t handle) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    destroyDieResources(it->second);
    _dice.erase(it);
}

void Renderer::destroyDieResources(DieInstance& die) {
    _scene->remove(die.entity);
    _engine->getRenderableManager().destroy(die.entity);
    utils::EntityManager::get().destroy(die.entity);
    if (die.vb)      _engine->destroy(die.vb);
    if (die.ib)      _engine->destroy(die.ib);
    if (die.matInst) _engine->destroy(die.matInst);
}

void Renderer::setDieTransform(uint32_t handle, const glm::quat& orientation,
                                const glm::vec3& position) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    it->second.position = position;

    auto& tcm = _engine->getTransformManager();
    auto inst = tcm.getInstance(it->second.entity);
    // Build transform matrix from quaternion + translation
    filament::math::mat4f T = filament::math::mat4f::translation(
        filament::math::float3{position.x, position.y, position.z});
    filament::math::mat4f R = filament::math::mat4f(filament::math::mat3f(
        filament::math::quatf{orientation.w, orientation.x, orientation.y, orientation.z}));
    tcm.setTransform(inst, T * R);
}

void Renderer::renderFrame() {
    if (!_swapChain) return;
    if (_renderer->beginFrame(_swapChain)) {
        _renderer->render(_view);
        _renderer->endFrame();
    }
}

#endif // DICE3D_HAVE_FILAMENT
