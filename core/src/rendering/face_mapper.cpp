#define GLM_ENABLE_EXPERIMENTAL
#include "dice3d/rendering/face_mapper.h"
#include <glm/gtx/quaternion.hpp>
using namespace dice3d;

FaceMapper::FaceMapper(const PolyMesh& mesh,
                       const glm::vec3& cameraForward,
                       const glm::vec3& cameraUp) {
    for (auto& face : mesh.faces) {
        if (face.faceNumber <= 0) continue;
        if (_orientations.count(face.faceNumber)) continue;  // first face wins
        _normals[face.faceNumber] = face.normal;
        _orientations[face.faceNumber] = basisToCamera(
            face.normal, face.up, cameraForward, cameraUp
        );
    }
}

glm::quat FaceMapper::orientationForFace(int number) const {
    return _orientations.at(number);  // throws std::out_of_range if missing
}

glm::vec3 FaceMapper::faceNormalForNumber(int number) const {
    return _normals.at(number);
}

glm::quat FaceMapper::basisToCamera(
    const glm::vec3& fn, const glm::vec3& fu,
    const glm::vec3& cf, const glm::vec3& cu)
{
    // Build rotation R such that:
    //   R * fn = -cf   (face normal points toward camera, i.e. into the viewer)
    //   R * fu ≈ cu    (face up aligns with camera up)
    //
    // Strategy: construct two orthonormal bases and build a rotation matrix
    // that maps from-basis to to-basis.

    // "From" basis: (faceNormal, faceUp_ortho, faceRight)
    glm::vec3 fn_n = glm::normalize(fn);
    glm::vec3 fu_n = glm::normalize(fu - fn_n * glm::dot(fn_n, fu));  // Gram-Schmidt
    if (glm::length(fu_n) < 1e-5f) {
        // fu is parallel to fn — pick any perpendicular
        glm::vec3 arb = std::abs(fn_n.x) < 0.9f ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
        fu_n = glm::normalize(arb - fn_n * glm::dot(fn_n, arb));
    } else {
        fu_n = glm::normalize(fu_n);
    }
    glm::vec3 fr_n = glm::normalize(glm::cross(fn_n, fu_n));

    // "To" basis: (-cameraForward, cameraUp_ortho, cameraRight)
    glm::vec3 tn = glm::normalize(-cf);
    glm::vec3 tu = glm::normalize(cu - tn * glm::dot(tn, cu));
    if (glm::length(tu) < 1e-5f) {
        glm::vec3 arb = std::abs(tn.x) < 0.9f ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
        tu = glm::normalize(arb - tn * glm::dot(tn, arb));
    } else {
        tu = glm::normalize(tu);
    }
    glm::vec3 tr = glm::normalize(glm::cross(tn, tu));

    // R = toBasis * transpose(fromBasis)
    // glm::mat3 columns are the basis vectors
    glm::mat3 fromBasis(fn_n, fu_n, fr_n);  // columns: from-normal, from-up, from-right
    glm::mat3 toBasis(tn, tu, tr);          // columns: to-normal, to-up, to-right
    glm::mat3 R = toBasis * glm::transpose(fromBasis);
    return glm::normalize(glm::quat_cast(R));
}
