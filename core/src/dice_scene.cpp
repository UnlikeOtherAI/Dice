#include "dice3d/dice_scene.h"
#include <cmath>
using namespace dice3d;

// Camera looks down -Z, up is +Y
static const glm::vec3 kCameraForward(0, 0, -1);
static const glm::vec3 kCameraUp(0, 1, 0);
static constexpr float kSelectionFlashDuration = 0.9f;
static constexpr float kSelectionFlashPulses = 3.0f;

static float selectionFlashIntensity(float elapsed) {
    float t = elapsed / kSelectionFlashDuration;
    if (t <= 0.0f || t >= 1.0f) return 0.0f;

    float wave = std::sin(t * kSelectionFlashPulses * 3.14159265358979323846f);
    float envelope = 1.0f - 0.20f * t;
    return wave * wave * envelope;
}

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

void DiceScene::setCameraDistance(float distance) {
    _renderer->setCameraDistance(distance);
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
        glm::vec3(0, 0, 0),
        config.presentationMode,
        config.idleSpinSpeed,
        config.presentationSpinSpeed,
        config.presentationDuration,
        config.dragSensitivity,
        config.selectionFlashEnabled
    };
    switch (config.presentationMode) {
    case PresentationMode::SpinIn:
        _dice[handle].anim->startPresentationSpin(config.presentationSpinSpeed, config.presentationDuration);
        break;
    case PresentationMode::IdleSpin:
        _dice[handle].anim->startIdleSpin(config.idleSpinSpeed);
        break;
    case PresentationMode::Static:
        break;
    }

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
    die.pendingFlashFace = result;
    die.flashingFace = 0;
    die.flashElapsed = 0.0f;
    _renderer->setDieFaceHighlight(die.renderHandle, 0, 0.0f);
    die.anim->roll(target, duration);
}

void DiceScene::setPresentationMode(uint32_t handle, PresentationMode mode, float speed, float duration) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    auto& die = it->second;
    die.presentationMode = mode;
    die.idleSpinSpeed = speed;
    die.presentationSpinSpeed = speed;
    die.presentationDuration = duration;
    switch (mode) {
    case PresentationMode::Static:
        die.anim->stopIdleSpin();
        break;
    case PresentationMode::SpinIn:
        die.anim->startPresentationSpin(speed, duration);
        break;
    case PresentationMode::IdleSpin:
        die.anim->startIdleSpin(speed);
        break;
    }
}

void DiceScene::setIdleSpinSpeed(uint32_t handle, float speed) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    it->second.idleSpinSpeed = speed;
    if (it->second.presentationMode == PresentationMode::IdleSpin) {
        it->second.anim->startIdleSpin(speed);
    }
}

void DiceScene::setSelectionFlashEnabled(uint32_t handle, bool enabled) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    auto& die = it->second;
    die.selectionFlashEnabled = enabled;
    if (!enabled) {
        die.pendingFlashFace = 0;
        die.flashingFace = 0;
        die.flashElapsed = 0.0f;
        _renderer->setDieFaceHighlight(die.renderHandle, 0, 0.0f);
    }
}

void DiceScene::beginDrag(uint32_t handle) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    it->second.anim->beginDrag();
}

void DiceScene::dragBy(uint32_t handle, float deltaX, float deltaY) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    const float s = it->second.dragSensitivity;
    it->second.anim->dragBy(deltaX * s, deltaY * s);
}

void DiceScene::endDrag(uint32_t handle) {
    auto it = _dice.find(handle);
    if (it == _dice.end()) return;
    it->second.anim->endDrag();
    if (it->second.presentationMode == PresentationMode::IdleSpin) {
        it->second.anim->startIdleSpin(it->second.idleSpinSpeed);
    }
}

void DiceScene::tick(float dt) {
    for (auto& [handle, die] : _dice) {
        auto previousState = die.anim->state();
        die.anim->tick(dt);
        auto currentState = die.anim->state();
        glm::quat orient = die.anim->currentOrientation();
        _renderer->setDieTransform(die.renderHandle, orient, die.position);

        if (previousState == AnimationController::State::Spinning &&
            currentState == AnimationController::State::Settled &&
            die.selectionFlashEnabled &&
            die.pendingFlashFace > 0) {
            die.flashingFace = die.pendingFlashFace;
            die.flashElapsed = 0.0f;
        }

        float flashIntensity = 0.0f;
        if (die.flashingFace > 0) {
            die.flashElapsed += dt;
            flashIntensity = selectionFlashIntensity(die.flashElapsed);
            if (die.flashElapsed >= kSelectionFlashDuration) {
                die.flashingFace = 0;
                die.flashElapsed = 0.0f;
                flashIntensity = 0.0f;
            }
        }
        _renderer->setDieFaceHighlight(die.renderHandle, die.flashingFace, flashIntensity);
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
