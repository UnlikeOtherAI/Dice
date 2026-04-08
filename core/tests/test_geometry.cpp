#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/geometry/polyhedra.h"
#include "dice3d/geometry/chamfer.h"
#include "dice3d/geometry/mesh_builder.h"
#include <cmath>
#include <set>
using namespace dice3d;

TEST_CASE("d6 has 8 vertices and 6 faces") {
    auto mesh = Polyhedra::generate(6);
    REQUIRE(mesh.vertices.size() == 8);
    REQUIRE(mesh.faces.size() == 6);
    for (auto& f : mesh.faces) REQUIRE(f.indices.size() == 4);
}

TEST_CASE("d4 has 4 vertices and 4 faces") {
    auto mesh = Polyhedra::generate(4);
    REQUIRE(mesh.vertices.size() == 4);
    REQUIRE(mesh.faces.size() == 4);
}

TEST_CASE("d8 has 6 vertices and 8 faces") {
    auto mesh = Polyhedra::generate(8);
    REQUIRE(mesh.vertices.size() == 6);
    REQUIRE(mesh.faces.size() == 8);
}

TEST_CASE("d10 has 12 vertices and 10 faces") {
    auto mesh = Polyhedra::generate(10);
    REQUIRE(mesh.vertices.size() == 12);
    REQUIRE(mesh.faces.size() == 10);
}

TEST_CASE("d10 edge lengths are approximately 0.618 and 1.618") {
    auto mesh = Polyhedra::generate(10);
    auto edgeLen = [&](int a, int b) {
        auto d = mesh.vertices[a] - mesh.vertices[b];
        return std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    };
    // F0 = {8, 2, 6, 11} after enforce-outward (or check both orderings)
    // The kite has short edge (apex->neighbor) and long edge (neighbor->opposite)
    // We check any edge pair in F0's original kite {8,2,6,11}
    float e1 = edgeLen(8, 2);
    float e2 = edgeLen(2, 6);
    // One should be ~0.618, other ~1.618
    float shorter = std::min(e1, e2);
    float longer  = std::max(e1, e2);
    REQUIRE_THAT(shorter, Catch::Matchers::WithinAbs(0.618f, 0.01f));
    REQUIRE_THAT(longer,  Catch::Matchers::WithinAbs(1.618f, 0.01f));
}

TEST_CASE("d12 has 20 vertices and 12 faces") {
    auto mesh = Polyhedra::generate(12);
    REQUIRE(mesh.vertices.size() == 20);
    REQUIRE(mesh.faces.size() == 12);
}

TEST_CASE("d20 has 12 vertices and 20 faces") {
    auto mesh = Polyhedra::generate(20);
    REQUIRE(mesh.vertices.size() == 12);
    REQUIRE(mesh.faces.size() == 20);
}

TEST_CASE("d16 has 10 vertices and 16 faces") {
    auto mesh = Polyhedra::generate(16);
    REQUIRE(mesh.vertices.size() == 10);
    REQUIRE(mesh.faces.size() == 16);
}

TEST_CASE("d32 pentakis dodecahedron has 32 vertices and 60 faces") {
    auto mesh = Polyhedra::generate(32);
    REQUIRE(mesh.vertices.size() == 32);
    REQUIRE(mesh.faces.size() == 60);
}

TEST_CASE("d32 covers at least 32 distinct face numbers") {
    auto mesh = Polyhedra::generate(32);
    std::set<int> nums;
    for (auto& f : mesh.faces) if (f.faceNumber > 0) nums.insert(f.faceNumber);
    REQUIRE(nums.size() >= 32);
    REQUIRE(*nums.begin() == 1);
    REQUIRE(*nums.rbegin() == 32);
}

TEST_CASE("d32 apexes sit outside their parent pentagon planes") {
    auto mesh = Polyhedra::generate(32);
    REQUIRE(mesh.vertices.size() == 32);
    REQUIRE(mesh.faces.size() == 60);

    for (int apex = 20; apex < 32; ++apex) {
        std::vector<const Face*> parentFaces;
        for (const auto& face : mesh.faces) {
            if (face.indices[0] == apex || face.indices[1] == apex || face.indices[2] == apex)
                parentFaces.push_back(&face);
        }

        REQUIRE(parentFaces.size() == 5);

        int a = -1;
        int b = -1;
        int c = -1;
        for (const auto* face : parentFaces) {
            for (int idx : face->indices) {
                if (idx == apex) continue;
                if (a == -1) { a = idx; continue; }
                if (b == -1 && idx != a) { b = idx; continue; }
                if (c == -1 && idx != a && idx != b) { c = idx; }
            }
            if (c != -1) break;
        }

        REQUIRE(a != -1);
        REQUIRE(b != -1);
        REQUIRE(c != -1);

        glm::vec3 centroid(0.0f);
        std::set<int> baseVertices;
        for (const auto* face : parentFaces) {
            for (int idx : face->indices) {
                if (idx != apex) baseVertices.insert(idx);
            }
        }
        REQUIRE(baseVertices.size() == 5);
        for (int idx : baseVertices) centroid += mesh.vertices[idx];
        centroid /= (float)baseVertices.size();

        glm::vec3 normal = glm::normalize(glm::cross(
            mesh.vertices[b] - mesh.vertices[a],
            mesh.vertices[c] - mesh.vertices[a]
        ));
        if (glm::dot(normal, centroid) < 0.0f) normal = -normal;

        float outwardOffset = glm::dot(mesh.vertices[apex] - centroid, normal);
        REQUIRE(outwardOffset > 0.1f);
    }
}

TEST_CASE("all face normals point outward from origin") {
    for (int sides : {4, 6, 8, 10, 12, 16, 20, 32}) {
        CAPTURE(sides);
        auto mesh = Polyhedra::generate(sides);
        for (auto& face : mesh.faces) {
            glm::vec3 centroid(0);
            for (int i : face.indices) centroid += mesh.vertices[i];
            centroid /= (float)face.indices.size();
            REQUIRE(glm::dot(face.normal, centroid) > 0.0f);
        }
    }
}

TEST_CASE("mesh builder produces non-empty vertex and index buffers") {
    auto base = Polyhedra::generate(6);
    auto chamfered = Chamfer::apply(base, 0.05f);
    auto mesh = MeshBuilder::build(chamfered, /*atlasSize=*/512);
    REQUIRE(!mesh.positions.empty());
    REQUIRE(!mesh.indices.empty());
    REQUIRE(mesh.uvs.size() == mesh.positions.size());
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    REQUIRE(mesh.indices.size() % 3 == 0);
}

TEST_CASE("all UV coordinates are in [0, 1] range") {
    auto base = Polyhedra::generate(20);
    auto mesh = MeshBuilder::build(Chamfer::apply(base, 0.05f), 512);
    for (auto& uv : mesh.uvs) {
        REQUIRE(uv.x >= 0.0f); REQUIRE(uv.x <= 1.0f);
        REQUIRE(uv.y >= 0.0f); REQUIRE(uv.y <= 1.0f);
    }
}
