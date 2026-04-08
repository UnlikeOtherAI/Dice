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
    for (auto& f : result.faces) {
        glm::vec3 c(0);
        for (int i : f.indices) c += result.vertices[i];
        c /= (float)f.indices.size();
        REQUIRE(glm::dot(f.normal, c) > 0.0f);
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
