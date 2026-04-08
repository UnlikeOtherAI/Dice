#include <catch2/catch_test_macros.hpp>
#include "dice3d/geometry/polyhedra.h"
#include "dice3d/geometry/chamfer.h"
using namespace dice3d;

TEST_CASE("chamfer with factor 0.0 produces same face count") {
    auto base = Polyhedra::generate(6);
    auto result = Chamfer::apply(base, 0.0f);
    REQUIRE(result.faces.size() == base.faces.size());
}

TEST_CASE("chamfer d6 with factor 0.05 adds edge and corner faces") {
    auto base = Polyhedra::generate(6);
    // d6: 6 faces, 12 edges, 8 vertices
    // chamfered: 6 original faces + 12 edge faces + 8 corner faces = 26 faces
    auto result = Chamfer::apply(base, 0.05f);
    REQUIRE(result.faces.size() == 26);
}

TEST_CASE("chamfer preserves outward normals") {
    auto base = Polyhedra::generate(20);
    auto result = Chamfer::apply(base, 0.05f);
    for (size_t i = 0; i < result.faces.size(); i++) {
        auto& f = result.faces[i];
        if (f.indices.size() < 3) continue;

        // Stored normal must point outward from origin
        glm::vec3 c(0);
        for (int idx : f.indices) c += result.vertices[idx];
        c /= (float)f.indices.size();
        REQUIRE(glm::dot(f.normal, c) > 0.0f);

        // Geometric normal from vertex winding must agree with stored normal
        glm::vec3 a = result.vertices[f.indices[0]];
        glm::vec3 b = result.vertices[f.indices[1]];
        glm::vec3 cv = result.vertices[f.indices[2]];
        glm::vec3 geom_normal = glm::normalize(glm::cross(b - a, cv - a));
        REQUIRE(glm::dot(geom_normal, f.normal) > 0.0f);
    }
}

TEST_CASE("chamfer preserves face numbers on original faces") {
    auto base = Polyhedra::generate(6);
    auto result = Chamfer::apply(base, 0.05f);
    // Original 6 faces should retain their face numbers
    int namedFaces = 0;
    for (auto& f : result.faces) {
        if (f.faceNumber > 0) namedFaces++;
    }
    REQUIRE(namedFaces == 6);
}
