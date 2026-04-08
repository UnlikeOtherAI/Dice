#include "dice3d/dice_scene.h"
#include <cmath>
using namespace dice3d;

// Camera looks down -Z, up is +Y
static const glm::vec3 kCameraForward(0, 0, -1);
static const glm::vec3 kCameraUp(0, 1, 0);

DiceScene::DiceScene(int backend)
#ifdef DICE3D_HAVE_FILAMENT
    : _renderer(std::make_unique<Renderer>(static_cast<filament::Engine::Backend>(backend)))
#else
    : _renderer(std::make_unique<Renderer>(backend))
#endif
{}

void DiceScene::loadMaterial(const void* data, size_t size) {
    _renderer->loadMaterial(data, size);
}

void DiceScene::loadAtlas(const void* rgbaData, uint32_t width, uint32_t height) {
    _renderer->loadAtlasTexture(rgbaData, width, height);
}

void DiceScene::attachSurface(void* nativeWindow, uint32_t w, uint32_t h) {
    _renderer->attachSurface(nativeWindow, w, h);
}

void DiceScene::detachSurface() {
    _renderer->detachSurface();
}

void DiceScene::resize(uint32_t w, uint32_t h) {
    _renderer->resize(w, h);
}

uint32_t DiceScene::addDie(const DieConfig& config) {
    // Build mesh: generate → chamfer → mesh_builder
    auto poly = Polyhedra::generate(config.sides);
    auto chamfered = Chamfer::apply(poly, config.bevelFactor);
    auto gpuMesh = MeshBuilder::build(chamfered);

    uint32_t renderHandle = _renderer->addDie(gpuMesh, config.dieColor, config.whiteNumbers);

    // Build FaceMapper for the chamfered mesh
    auto faceMap = std::make_unique<FaceMapper>(chamfered, kCameraForward, kCameraUp);

    uint32_t handle = _nextHandle++;
    _dice[handle] = Die{
        renderHandle,
        std::make_unique<AnimationController>(),
        std::move(faceMap),
        glm::vec3(0, 0, 0)
    };

    layoutDice();
    return handle;
}

void DiceScene::removeDie(uint32_t handle) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    _renderer->removeDie(it->second.renderHandle);
    _dice.erase(it);
    layoutDice();
}

void DiceScene::roll(uint32_t handle, int result, float duration) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    auto& die = it->second;
    glm::quat target = die.faceMap->orientationForFace(result);
    die.anim->roll(target, duration);
}

void DiceScene::tick(float dt) {
    for (auto& [handle, die] : _dice) {
        die.anim->tick(dt);
        glm::quat orient = die.anim->currentOrientation();
        _renderer->setDieTransform(die.renderHandle, orient, die.position);
    }
}

void DiceScene::renderFrame() {
    _renderer->renderFrame();
}

void DiceScene::rollAll(const std::vector<std::pair<uint32_t, int>>& rolls, float duration) {
    for (auto& [handle, result] : rolls) {
        roll(handle, result, duration);
    }
}

void DiceScene::layoutDice() {
    // Arrange dice in a horizontal row centered at origin.
    // Spacing: 2.5 units apart.
    const float spacing = 2.5f;
    int n = (int)_dice.size();
    float offset = -(n - 1) * spacing * 0.5f;
    int i = 0;
    for (auto& [handle, die] : _dice) {
        die.position = glm::vec3(offset + i * spacing, 0.0f, 0.0f);
        i++;
    }
}
