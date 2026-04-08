#pragma once
#include "dice3d/geometry/polyhedra.h"
#include <unordered_map>
#include <glm/gtc/quaternion.hpp>

namespace dice3d {

class FaceMapper {
public:
    FaceMapper(const PolyMesh& mesh,
               const glm::vec3& cameraForward,
               const glm::vec3& cameraUp);

    // Returns quaternion to apply to die so face `number` faces the camera.
    // Throws std::out_of_range if number not found.
    glm::quat orientationForFace(int number) const;
    glm::vec3 faceNormalForNumber(int number) const;

private:
    std::unordered_map<int, glm::quat> _orientations;
    std::unordered_map<int, glm::vec3> _normals;

    // Build rotation R such that R*faceNormal = -cameraForward
    // and R*faceUp aligns with cameraUp
    static glm::quat basisToCamera(
        const glm::vec3& faceNormal,
        const glm::vec3& faceUp,
        const glm::vec3& camForward,
        const glm::vec3& camUp
    );
};

} // namespace dice3d
