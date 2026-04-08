#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/geometry/polyhedra.h"
#include "dice3d/geometry/chamfer.h"
#include "dice3d/rendering/face_mapper.h"
#include <cmath>
using namespace dice3d;

static const glm::vec3 kCameraDir(0,0,-1);  // camera looks down -Z
static const glm::vec3 kCameraUp(0,1,0);

TEST_CASE("face mapper has entry for every face number in d6") {
    auto mesh = Chamfer::apply(Polyhedra::generate(6), 0.05f);
    FaceMapper mapper(mesh, kCameraDir, kCameraUp);
    for (int n = 1; n <= 6; n++) {
        REQUIRE_NOTHROW(mapper.orientationForFace(n));
    }
}

TEST_CASE("face mapper: applying quaternion makes face normal point toward camera") {
    auto mesh = Chamfer::apply(Polyhedra::generate(6), 0.05f);
    FaceMapper mapper(mesh, kCameraDir, kCameraUp);
    for (int n = 1; n <= 6; n++) {
        glm::quat q = mapper.orientationForFace(n);
        glm::vec3 faceNormal = mapper.faceNormalForNumber(n);
        glm::vec3 rotated = q * faceNormal;
        // Should point toward camera (opposite of -Z camera dir = +Z)
        float d = glm::dot(rotated, -kCameraDir);
        REQUIRE_THAT(d, Catch::Matchers::WithinAbs(1.0f, 0.01f));
    }
}

TEST_CASE("face mapper: applying quaternion keeps face up aligned with camera up") {
    auto mesh = Chamfer::apply(Polyhedra::generate(20), 0.05f);
    FaceMapper mapper(mesh, kCameraDir, kCameraUp);
    for (const auto& face : mesh.faces) {
        if (face.faceNumber <= 0) continue;
        glm::quat q = mapper.orientationForFace(face.faceNumber);
        glm::vec3 rotatedNormal = q * face.normal;
        glm::vec3 rotatedUp = glm::normalize(q * face.up);
        glm::vec3 upOnFacePlane = glm::normalize(
            rotatedUp - rotatedNormal * glm::dot(rotatedNormal, rotatedUp)
        );
        float d = glm::dot(upOnFacePlane, -kCameraUp);
        REQUIRE_THAT(d, Catch::Matchers::WithinAbs(1.0f, 0.01f));
    }
}

TEST_CASE("face mapper d20 has entries for all 20 faces") {
    auto mesh = Chamfer::apply(Polyhedra::generate(20), 0.05f);
    FaceMapper mapper(mesh, kCameraDir, kCameraUp);
    for (int n = 1; n <= 20; n++)
        REQUIRE_NOTHROW(mapper.orientationForFace(n));
}
