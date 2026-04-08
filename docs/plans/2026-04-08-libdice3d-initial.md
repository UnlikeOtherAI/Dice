# libdice3d Initial Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a cross-platform C++ library that renders floating, physics-free 3D polyhedral dice with PBR materials, parametric edge beveling, and predetermined-outcome spin animation, wrapped for Swift (iOS) and Kotlin (Android).

**Architecture:** C++ core (`libdice3d`) owns all Filament rendering and animation. It exposes a pure-C ABI (`dice3d.h`) so iOS and Android can bind without C++ interop friction. Geometry is generated procedurally with a parametric chamfer. Animation is pure quaternion math — no physics engine.

**Tech Stack:** C++17, Google Filament 1.71+ (Metal/Vulkan/GLES), CMake 3.24+, XCFramework (iOS), AAR/JNI (Android), Swift 5.9+ (iOS wrapper), Kotlin (Android wrapper).

---

## Reference Documents

Read these before implementing anything:
- `docs/brief.md` — requirements, all resolved decisions
- `docs/research/filament-cross-platform-dice.md` — Filament integration patterns, gotchas
- `docs/research/closing-key-gaps.md` — iOS bridging, d10 exact coords, distribution

---

## Repository Layout

```
dice/
  CMakeLists.txt                  # root superbuild
  core/
    CMakeLists.txt
    include/dice3d/
      dice3d.h                    # public C ABI
    src/
      geometry/
        polyhedra.h / .cpp        # base polyhedra vertex data
        chamfer.h / .cpp          # edge-bevel algorithm
        mesh_builder.h / .cpp     # assembles VertexBuffer/IndexBuffer
      animation/
        quaternion_utils.h / .cpp
        animation_controller.h / .cpp
      rendering/
        renderer.h / .cpp         # Filament scene ownership
        face_mapper.h / .cpp
      dice_scene.h / .cpp         # top-level: owns N dice
      dice3d_impl.cpp             # C ABI implementation
    tests/
      test_geometry.cpp
      test_chamfer.cpp
      test_animation.cpp
      test_face_mapper.cpp
  platform/
    ios/
      DiceView.h / .mm            # Obj-C++ UIView
      DiceRenderer.h / .mm        # Obj-C++ Filament host
    android/
      CMakeLists.txt
      dice3d_jni.cpp
      src/main/java/com/dice3d/
        DiceView.kt
        DiceRenderer.kt
  wrapper/
    swift/
      Sources/Dice3D/
        DiceView.swift
        DiceController.swift
    kotlin/
      lib/src/main/kotlin/com/dice3d/
        DiceView.kt
  scripts/
    build_ios.sh
    build_android.sh
    build_xcframework.sh
```

---

## Task 1: CMake project skeleton + Catch2 test harness

**Files:**
- Create: `CMakeLists.txt`
- Create: `core/CMakeLists.txt`
- Create: `core/tests/test_geometry.cpp`

**Step 1: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.24)
project(dice3d VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)
```

**Step 2: Create core/CMakeLists.txt**

```cmake
# --- Filament paths (set by build script per platform) ---
# FILAMENT_DIR must be set: -DFILAMENT_DIR=/path/to/filament
if(NOT DEFINED FILAMENT_DIR)
    message(FATAL_ERROR "FILAMENT_DIR must be set")
endif()

# --- Library target ---
add_library(dice3d STATIC
    src/geometry/polyhedra.cpp
    src/geometry/chamfer.cpp
    src/geometry/mesh_builder.cpp
    src/animation/quaternion_utils.cpp
    src/animation/animation_controller.cpp
    src/rendering/renderer.cpp
    src/rendering/face_mapper.cpp
    src/dice_scene.cpp
    src/dice3d_impl.cpp
)
target_include_directories(dice3d PUBLIC include)
target_include_directories(dice3d PRIVATE ${FILAMENT_DIR}/include)

# --- Tests ---
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(dice3d_tests
        tests/test_geometry.cpp
        tests/test_chamfer.cpp
        tests/test_animation.cpp
        tests/test_face_mapper.cpp
    )
    target_link_libraries(dice3d_tests PRIVATE dice3d Catch2::Catch2WithMain)
    include(CTest)
    include(Catch)
    catch_discover_tests(dice3d_tests)
endif()
```

**Step 3: Stub all source files so it compiles**

Create each `.h` and `.cpp` listed above as empty stubs:
```cpp
// polyhedra.h — empty for now
#pragma once
```

**Step 4: Write a trivial passing test to confirm harness works**

```cpp
// core/tests/test_geometry.cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("sanity") {
    REQUIRE(1 + 1 == 2);
}
```

**Step 5: Build and run**

```bash
cd /System/Volumes/Data/.internal/projects/Projects/dice
cmake -B build -DBUILD_TESTING=ON -DFILAMENT_DIR=/tmp/filament-stub
cmake --build build
cd build && ctest --output-on-failure
```

Expected: 1 test passes.

**Step 6: Commit**

```bash
git init
git add .
git commit -m "chore: initial CMake skeleton and Catch2 test harness"
```

---

## Task 2: Base polyhedra — vertex and face data

**Files:**
- Modify: `core/include/dice3d/geometry/polyhedra.h`
- Modify: `core/src/geometry/polyhedra.cpp`
- Modify: `core/tests/test_geometry.cpp`

The geometry module returns raw vertex positions and face index lists. It does NOT apply chamfer or generate VertexBuffers yet — that is Task 3 and 4.

**Step 1: Write failing tests**

```cpp
// test_geometry.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/geometry/polyhedra.h"
using namespace dice3d;

TEST_CASE("d6 has 8 vertices and 6 faces") {
    auto mesh = Polyhedra::generate(6);
    REQUIRE(mesh.vertices.size() == 8);
    REQUIRE(mesh.faces.size() == 6);
    // each face is a quad (4 indices)
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
    // Verify McCooey canonical edge lengths
    std::set<float> lengths;
    // spot check F0 = {8, 2, 6, 11}: short edge 8->2, long edge 2->6
    auto edgeLen = [&](int a, int b) {
        auto d = mesh.vertices[a] - mesh.vertices[b];
        return std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    };
    float short_edge = edgeLen(8, 2);
    float long_edge  = edgeLen(2, 6);
    REQUIRE_THAT(short_edge, Catch::Matchers::WithinAbs(0.618f, 0.01f));
    REQUIRE_THAT(long_edge,  Catch::Matchers::WithinAbs(1.618f, 0.01f));
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
    REQUIRE(mesh.vertices.size() == 10);   // 8 equator + 2 apices
    REQUIRE(mesh.faces.size() == 16);
}

TEST_CASE("d32 pentakis dodecahedron has 32 vertices and 60 faces") {
    auto mesh = Polyhedra::generate(32);
    REQUIRE(mesh.vertices.size() == 32);
    REQUIRE(mesh.faces.size() == 60);
}

TEST_CASE("all face normals point outward from origin") {
    for (int sides : {4, 6, 8, 10, 12, 16, 20, 32}) {
        auto mesh = Polyhedra::generate(sides);
        for (auto& face : mesh.faces) {
            glm::vec3 centroid(0);
            for (int i : face.indices) centroid += mesh.vertices[i];
            centroid /= face.indices.size();
            REQUIRE(glm::dot(face.normal, centroid) > 0.0f);
        }
    }
}
```

**Step 2: Run — expect FAIL (not implemented)**

```bash
cd build && ctest --output-on-failure -R test_geometry
```

**Step 3: Write the header**

```cpp
// core/include/dice3d/geometry/polyhedra.h
#pragma once
#include <vector>
#include <glm/glm.hpp>  // Filament ships glm; use that

namespace dice3d {

struct Face {
    std::vector<int> indices;   // vertex indices (3 for triangles, 4 for quads)
    glm::vec3 normal;           // outward unit normal
    glm::vec3 up;               // in-face "up" for text orientation
    int faceNumber;             // die face value (1-based)
};

struct PolyMesh {
    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;
};

class Polyhedra {
public:
    // Generate canonical base mesh for the given die type.
    // Supported: 4, 6, 8, 10, 12, 16, 20, 32
    static PolyMesh generate(int sides);

private:
    static PolyMesh d4();
    static PolyMesh d6();
    static PolyMesh d8();
    static PolyMesh d10();
    static PolyMesh d12();
    static PolyMesh d16();
    static PolyMesh d20();
    static PolyMesh d32();

    // Enforce outward normals: flip face if dot(normal, centroid) < 0
    static void enforceOutwardNormals(PolyMesh& mesh);
    // Compute face normal from first 3 vertices (right-hand rule)
    static glm::vec3 faceNormal(const PolyMesh& mesh, const Face& face);
    // Compute face centroid
    static glm::vec3 faceCentroid(const PolyMesh& mesh, const Face& face);
};

} // namespace dice3d
```

**Step 4: Implement polyhedra.cpp**

Key data to implement (see research docs for full vertex lists):

```cpp
// core/src/geometry/polyhedra.cpp
#include "dice3d/geometry/polyhedra.h"
#include <stdexcept>
#include <cmath>
using namespace dice3d;

static const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;  // golden ratio

PolyMesh Polyhedra::generate(int sides) {
    switch (sides) {
        case 4:  return d4();
        case 6:  return d6();
        case 8:  return d8();
        case 10: return d10();
        case 12: return d12();
        case 16: return d16();
        case 20: return d20();
        case 32: return d32();
        default: throw std::invalid_argument("Unsupported die type: " + std::to_string(sides));
    }
}

// --- d6: cube, vertices at ±1 ---
PolyMesh Polyhedra::d6() {
    PolyMesh m;
    m.vertices = {
        {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},  // back face
        {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},  // front face
    };
    // 6 quad faces, CCW from outside: +Z, -Z, +Y, -Y, +X, -X
    m.faces = {
        {{4,5,6,7}, {0,0,1}, {0,1,0}, 1},   // front  +Z
        {{1,0,3,2}, {0,0,-1},{0,1,0}, 2},   // back   -Z
        {{3,7,6,2}, {0,1,0}, {0,0,-1},3},   // top    +Y
        {{0,1,5,4}, {0,-1,0},{0,0,1}, 4},   // bottom -Y
        {{1,2,6,5}, {1,0,0}, {0,1,0}, 5},   // right  +X
        {{0,4,7,3}, {-1,0,0},{0,1,0}, 6},   // left   -X
    };
    enforceOutwardNormals(m);
    return m;
}

// --- d10: exact McCooey coordinates ---
PolyMesh Polyhedra::d10() {
    const double C0 = (std::sqrt(5.0) - 1.0) / 4.0;
    const double C1 = (1.0 + std::sqrt(5.0)) / 4.0;
    const double C2 = (3.0 + std::sqrt(5.0)) / 4.0;

    PolyMesh m;
    m.vertices = {
        { 0.0f, (float)C0,  (float)C1},  // V0
        { 0.0f, (float)C0, -(float)C1},  // V1
        { 0.0f,-(float)C0,  (float)C1},  // V2
        { 0.0f,-(float)C0, -(float)C1},  // V3
        { 0.5f,  0.5f,  0.5f},           // V4
        { 0.5f,  0.5f, -0.5f},           // V5
        {-0.5f, -0.5f,  0.5f},           // V6
        {-0.5f, -0.5f, -0.5f},           // V7
        {(float)C2, -(float)C1, 0.0f},   // V8
        {-(float)C2,(float)C1, 0.0f},    // V9
        {(float)C0, (float)C1, 0.0f},    // V10
        {-(float)C0,-(float)C1, 0.0f},   // V11
    };
    // 10 kite faces {apex, a, opposite, b}
    // up = normalize(opposite - apex projected onto face plane)
    struct KiteFace { int apex, a, opp, b; int number; };
    std::vector<KiteFace> kites = {
        {8, 2, 6, 11, 1}, {8, 11, 7, 3, 2}, {8, 3, 1, 5, 3},
        {8, 5, 10, 4, 4}, {8, 4, 0, 2, 5},
        {9, 0, 4, 10, 6}, {9, 10, 5, 1, 7}, {9, 1, 3, 7, 8},
        {9, 7, 11, 6, 9}, {9, 6, 2, 0, 10},
    };
    for (auto& k : kites) {
        Face f;
        f.indices = {k.apex, k.a, k.opp, k.b};
        f.faceNumber = k.number;
        // compute normal and up after adding
        m.faces.push_back(f);
    }
    enforceOutwardNormals(m);
    // compute up vectors
    for (auto& f : m.faces) {
        glm::vec3 apex = m.vertices[f.indices[0]];
        glm::vec3 opp  = m.vertices[f.indices[2]];
        glm::vec3 u_raw = opp - apex;
        glm::vec3 n = f.normal;
        f.up = glm::normalize(u_raw - n * glm::dot(n, u_raw));
    }
    return m;
}

// --- d16: octagonal dipyramid ---
PolyMesh Polyhedra::d16() {
    PolyMesh m;
    const int N = 8;
    const float R = 1.0f, H = 1.2f;  // radius and apex height
    // Equator vertices
    for (int k = 0; k < N; k++) {
        float angle = 2.0f * M_PI * k / N;
        m.vertices.push_back({R * std::cos(angle), R * std::sin(angle), 0.0f});
    }
    m.vertices.push_back({0.0f, 0.0f,  H});  // top apex: index N
    m.vertices.push_back({0.0f, 0.0f, -H});  // bottom apex: index N+1

    int top = N, bot = N + 1;
    for (int k = 0; k < N; k++) {
        int next = (k + 1) % N;
        // top triangle
        m.faces.push_back({{top, k, next}, {}, {}, k + 1});
        // bottom triangle (reversed winding)
        m.faces.push_back({{bot, next, k}, {}, {}, k + 1 + N});
    }
    enforceOutwardNormals(m);
    for (auto& f : m.faces) {
        f.normal = faceNormal(m, f);
        // up: project world +Y onto face plane
        glm::vec3 worldUp(0, 1, 0);
        glm::vec3 n = f.normal;
        glm::vec3 u = worldUp - n * glm::dot(n, worldUp);
        if (glm::length(u) < 1e-4f) u = glm::vec3(1, 0, 0);  // fallback
        f.up = glm::normalize(u);
    }
    return m;
}

// --- d32: pentakis dodecahedron (20 dodeca verts + 12 apex verts = 32, 60 triangles) ---
PolyMesh Polyhedra::d32() {
    // Dodecahedron base vertices (20 verts, 12 faces of 5 indices each)
    // Build from known coordinates using golden ratio
    PolyMesh m;
    const float p = PHI;
    const float ip = 1.0f / PHI;
    // 20 dodecahedron vertices
    m.vertices = {
        // (±1, ±1, ±1)
        { 1, 1, 1}, { 1, 1,-1}, { 1,-1, 1}, { 1,-1,-1},
        {-1, 1, 1}, {-1, 1,-1}, {-1,-1, 1}, {-1,-1,-1},
        // (0, ±1/φ, ±φ)
        { 0, ip, p}, { 0, ip,-p}, { 0,-ip, p}, { 0,-ip,-p},
        // (±1/φ, ±φ, 0)
        { ip, p, 0}, {-ip, p, 0}, { ip,-p, 0}, {-ip,-p, 0},
        // (±φ, 0, ±1/φ)
        { p, 0, ip}, { p, 0,-ip}, {-p, 0, ip}, {-p, 0,-ip},
    };
    // 12 pentagonal face index lists
    std::vector<std::vector<int>> dodecaFaces = {
        {0,8,4,13,12}, {0,12,1,17,16}, {0,16,2,10,8},
        {1,12,13,5,9},  {1,9,11,3,17},  {2,16,17,3,14},
        {2,14,15,6,10}, {3,11,7,15,14}, {4,8,10,6,18},
        {4,18,19,5,13}, {5,19,7,11,9},  {6,15,7,19,18},
    };
    // Generate 12 apex vertices (face centroid pushed out along normal)
    const float apexScale = 1.15f;  // mild outward push for rounded look
    for (auto& fi : dodecaFaces) {
        glm::vec3 centroid(0);
        for (int i : fi) centroid += m.vertices[i];
        centroid /= fi.size();
        m.vertices.push_back(glm::normalize(centroid) * apexScale);
    }
    // Triangulate: for each pentagon + apex, create 5 triangles
    int faceNum = 1;
    for (int f = 0; f < 12; f++) {
        int apex = 20 + f;
        auto& fi = dodecaFaces[f];
        for (int i = 0; i < 5; i++) {
            int a = fi[i], b = fi[(i+1)%5];
            m.faces.push_back({{apex, a, b}, {}, {}, faceNum});
        }
        faceNum++;
        if (faceNum > 32) faceNum = 1;  // wrap: 12*5=60 faces, values 1-32 repeat
    }
    enforceOutwardNormals(m);
    return m;
}

void Polyhedra::enforceOutwardNormals(PolyMesh& mesh) {
    for (auto& f : mesh.faces) {
        f.normal = faceNormal(mesh, f);
        glm::vec3 c = faceCentroid(mesh, f);
        if (glm::dot(f.normal, c) < 0.0f) {
            std::reverse(f.indices.begin(), f.indices.end());
            f.normal = -f.normal;
        }
    }
}

glm::vec3 Polyhedra::faceNormal(const PolyMesh& mesh, const Face& face) {
    auto& v = mesh.vertices;
    auto& idx = face.indices;
    glm::vec3 a = v[idx[1]] - v[idx[0]];
    glm::vec3 b = v[idx[2]] - v[idx[0]];
    return glm::normalize(glm::cross(a, b));
}

glm::vec3 Polyhedra::faceCentroid(const PolyMesh& mesh, const Face& face) {
    glm::vec3 c(0);
    for (int i : face.indices) c += mesh.vertices[i];
    return c / (float)face.indices.size();
}
```

> **Note:** d4, d8, d12, d20 follow the same pattern using known Platonic solid coordinates. Implement them using standard coordinate tables (tetrahedron, octahedron, dodecahedron, icosahedron). All are in every 3D geometry textbook and widely available.

**Step 5: Run tests — expect pass**

```bash
cd build && ctest --output-on-failure -R test_geometry
```

Expected: all geometry tests pass.

**Step 6: Commit**

```bash
git add core/include/dice3d/geometry/ core/src/geometry/polyhedra.cpp core/tests/test_geometry.cpp
git commit -m "feat: base polyhedra geometry (d4/d6/d8/d10/d12/d16/d20/d32)"
```

---

## Task 3: Parametric edge chamfering (rounded corners)

**Files:**
- Create: `core/include/dice3d/geometry/chamfer.h`
- Create: `core/src/geometry/chamfer.cpp`
- Create: `core/tests/test_chamfer.cpp`

The chamfer algorithm takes a `PolyMesh` and a `float bevel_factor` (default 0.05) and returns a new `PolyMesh` with:
1. Each original face shrunk inward (inset toward centroid)
2. New quad faces filling the gaps between adjacent inset faces (one per original edge)
3. New polygon caps at each original vertex

**Step 1: Write failing tests**

```cpp
// core/tests/test_chamfer.cpp
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
        c /= f.indices.size();
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
```

**Step 2: Run — expect FAIL**

**Step 3: Implement chamfer.h**

```cpp
// core/include/dice3d/geometry/chamfer.h
#pragma once
#include "polyhedra.h"

namespace dice3d {

class Chamfer {
public:
    // Apply parametric edge bevel to a PolyMesh.
    // bevel_factor: 0.0 = no bevel, 0.05 = default mild, 0.15 = maximum
    static PolyMesh apply(const PolyMesh& input, float bevel_factor);
};

} // namespace dice3d
```

**Step 4: Implement chamfer.cpp — face-inset algorithm**

```cpp
// core/src/geometry/chamfer.cpp
#include "dice3d/geometry/chamfer.h"
#include <map>
#include <set>
using namespace dice3d;

PolyMesh Chamfer::apply(const PolyMesh& input, float bevel_factor) {
    if (bevel_factor <= 0.0f) return input;

    PolyMesh result;

    // For each original face, create an inset face:
    // Move each face vertex toward face centroid by bevel_factor
    // New vertex index: inset_start + (face_index * face_vertex_count + local_vert)
    std::vector<std::vector<int>> insetFaceVerts(input.faces.size());

    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        const Face& face = input.faces[fi];
        glm::vec3 centroid(0);
        for (int vi : face.indices) centroid += input.vertices[vi];
        centroid /= face.indices.size();

        Face insetFace;
        insetFace.normal = face.normal;
        insetFace.up = face.up;
        insetFace.faceNumber = face.faceNumber;

        for (int vi : face.indices) {
            glm::vec3 orig = input.vertices[vi];
            glm::vec3 inset = orig + (centroid - orig) * bevel_factor;
            insetFaceVerts[fi].push_back((int)result.vertices.size());
            result.vertices.push_back(inset);
            insetFace.indices.push_back((int)result.vertices.size() - 1);
        }
        result.faces.push_back(insetFace);
    }

    // Build edge map: for each shared edge (vi, vj) in original mesh,
    // find the two adjacent faces and connect their inset vertices with a quad
    // Edge key: pair<min, max> -> list of (faceIndex, localVertexA, localVertexB)
    struct EdgeRef { int faceIdx, localA, localB; };
    std::map<std::pair<int,int>, std::vector<EdgeRef>> edgeMap;

    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        auto& idx = input.faces[fi].indices;
        for (int k = 0; k < (int)idx.size(); k++) {
            int a = idx[k], b = idx[(k+1) % idx.size()];
            auto key = std::make_pair(std::min(a,b), std::max(a,b));
            edgeMap[key].push_back({fi, k, (k+1) % (int)idx.size()});
        }
    }

    for (auto& [edge, refs] : edgeMap) {
        if (refs.size() != 2) continue;  // boundary edge, skip
        auto& r0 = refs[0]; auto& r1 = refs[1];
        // r0 sees edge as (localA -> localB), r1 sees it reversed
        int v0a = insetFaceVerts[r0.faceIdx][r0.localA];
        int v0b = insetFaceVerts[r0.faceIdx][r0.localB];
        int v1a = insetFaceVerts[r1.faceIdx][r1.localA];
        int v1b = insetFaceVerts[r1.faceIdx][r1.localB];
        // Bevel quad: v0a, v0b on one face; v1b, v1a on adjacent (reversed)
        Face bevel;
        bevel.indices = {v0a, v0b, v1a, v1b};
        bevel.faceNumber = 0;  // bevel faces have no die number
        // Normal: average of the two adjacent face normals
        bevel.normal = glm::normalize(
            input.faces[r0.faceIdx].normal + input.faces[r1.faceIdx].normal
        );
        bevel.up = glm::vec3(0,1,0);  // approximate; will be recomputed
        result.faces.push_back(bevel);
    }

    // Vertex corner caps: for each original vertex, collect all inset vertices
    // that originated from it, sorted around the vertex, and add a polygon
    std::map<int, std::vector<int>> vertexToInset;
    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        auto& orig_idx = input.faces[fi].indices;
        for (int k = 0; k < (int)orig_idx.size(); k++) {
            vertexToInset[orig_idx[k]].push_back(insetFaceVerts[fi][k]);
        }
    }

    for (auto& [origVert, insetVerts] : vertexToInset) {
        if (insetVerts.size() < 3) continue;
        // Sort inset verts by angle around original vertex
        glm::vec3 center = input.vertices[origVert];
        // Project onto plane perpendicular to centroid direction
        glm::vec3 normal = glm::normalize(center);
        // Compute angles and sort
        glm::vec3 ref = glm::normalize(result.vertices[insetVerts[0]] - center);
        std::vector<std::pair<float, int>> angleIdx;
        for (int iv : insetVerts) {
            glm::vec3 d = glm::normalize(result.vertices[iv] - center);
            float angle = std::atan2(
                glm::dot(glm::cross(ref, d), normal),
                glm::dot(ref, d)
            );
            angleIdx.push_back({angle, iv});
        }
        std::sort(angleIdx.begin(), angleIdx.end());

        Face cap;
        for (auto& [a, iv] : angleIdx) cap.indices.push_back(iv);
        cap.normal = glm::normalize(center);
        cap.faceNumber = 0;
        cap.up = glm::vec3(0,1,0);
        result.faces.push_back(cap);
    }

    return result;
}
```

**Step 5: Run tests — expect pass**

```bash
cd build && ctest --output-on-failure -R test_chamfer
```

**Step 6: Commit**

```bash
git add core/include/dice3d/geometry/chamfer.h core/src/geometry/chamfer.cpp core/tests/test_chamfer.cpp
git commit -m "feat: parametric edge chamfer for rounded dice geometry"
```

---

## Task 4: Mesh builder — convert PolyMesh to Filament VertexBuffer/IndexBuffer

**Files:**
- Create: `core/include/dice3d/geometry/mesh_builder.h`
- Create: `core/src/geometry/mesh_builder.cpp`

This task converts the abstract `PolyMesh` into triangulated GPU-ready data: expanded vertices (one per face per vertex, so each face has independent UVs), UV atlas coordinates, and tangent quaternions.

**Step 1: Write tests**

```cpp
// add to test_geometry.cpp
#include "dice3d/geometry/mesh_builder.h"

TEST_CASE("mesh builder produces non-empty vertex and index buffers") {
    auto base = Polyhedra::generate(6);
    auto chamfered = Chamfer::apply(base, 0.05f);
    auto mesh = MeshBuilder::build(chamfered, /*atlasSize=*/512);
    REQUIRE(!mesh.positions.empty());
    REQUIRE(!mesh.indices.empty());
    REQUIRE(mesh.uvs.size() == mesh.positions.size());
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    // index count must be multiple of 3 (triangles)
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
```

**Step 2: Implement mesh_builder.h**

```cpp
// core/include/dice3d/geometry/mesh_builder.h
#pragma once
#include "polyhedra.h"
#include <vector>

namespace dice3d {

struct GpuMesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;  // Filament tangent quaternion (xyzw)
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t>  indices;
    // face number -> UV cell rect (u0, v0, u1, v1) in atlas
    // faceNumber 0 = bevel face, no label
    std::vector<std::pair<int, glm::vec4>> faceAtlasCells;
};

class MeshBuilder {
public:
    // Build GPU mesh from chamfered PolyMesh.
    // atlasSize: texture atlas width/height in pixels (square)
    static GpuMesh build(const PolyMesh& mesh, int atlasSize = 512);
};

} // namespace dice3d
```

**Step 3: Implement mesh_builder.cpp**

Key operations:
1. Triangulate each face (fan from vertex 0)
2. Assign UV cell in atlas: number faces get a labeled cell, bevel faces get a solid-color cell
3. Compute per-vertex UV within the cell
4. Compute tangent quaternions using Filament's convention (TBN -> quaternion)

```cpp
// core/src/geometry/mesh_builder.cpp
// (full implementation — triangulate faces, UV atlas layout, tangent generation)
// See docs/research/filament-cross-platform-dice.md for UV atlas guidance
// and tangent-as-quaternion encoding requirement.
```

> The full implementation is mechanical (triangulate fan, UV pack, TBN->quat). Write it, verifying with the UV range test above.

**Step 4: Commit**

```bash
git commit -m "feat: mesh builder — triangulation, UV atlas, tangent generation"
```

---

## Task 5: Quaternion animation controller

**Files:**
- Create: `core/include/dice3d/animation/quaternion_utils.h`
- Create: `core/src/animation/quaternion_utils.cpp`
- Create: `core/include/dice3d/animation/animation_controller.h`
- Create: `core/src/animation/animation_controller.cpp`
- Create: `core/tests/test_animation.cpp`

**Step 1: Write failing tests**

```cpp
// core/tests/test_animation.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/animation/quaternion_utils.h"
#include "dice3d/animation/animation_controller.h"
using namespace dice3d;

TEST_CASE("slerp at t=0 returns qA") {
    glm::quat a(1,0,0,0), b(0,1,0,0);
    auto result = QuaternionUtils::slerp(a, b, 0.0f);
    REQUIRE_THAT(result.w, Catch::Matchers::WithinAbs(1.0f, 0.001f));
}

TEST_CASE("slerp at t=1 returns qB") {
    glm::quat a(1,0,0,0), b = glm::normalize(glm::quat(0,1,0,0));
    auto result = QuaternionUtils::slerp(a, b, 1.0f);
    REQUIRE_THAT(std::abs(result.w), Catch::Matchers::WithinAbs(std::abs(b.w), 0.001f));
}

TEST_CASE("slerp flips qB if dot < 0 (shortest path)") {
    glm::quat a(1,0,0,0);
    glm::quat b(-1,0,0,0);  // same rotation, opposite hemisphere
    // Should NOT spin the long way around — result should be near identity
    auto result = QuaternionUtils::slerp(a, b, 0.5f);
    REQUIRE_THAT(result.w, Catch::Matchers::WithinAbs(1.0f, 0.01f));
}

TEST_CASE("easeOutQuint at t=0 is 0, t=1 is 1") {
    REQUIRE_THAT(QuaternionUtils::easeOutQuint(0.0f), Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(QuaternionUtils::easeOutQuint(1.0f), Catch::Matchers::WithinAbs(1.0f, 0.001f));
}

TEST_CASE("animation starts in idle state") {
    AnimationController ctrl;
    REQUIRE(ctrl.state() == AnimationController::State::Idle);
}

TEST_CASE("after roll(), state is Spinning") {
    AnimationController ctrl;
    glm::quat target(1,0,0,0);
    ctrl.roll(target, 2.0f);
    REQUIRE(ctrl.state() == AnimationController::State::Spinning);
}

TEST_CASE("after full duration, state is Settled") {
    AnimationController ctrl;
    glm::quat target = glm::normalize(glm::quat(0.5f, 0.5f, 0.5f, 0.5f));
    ctrl.roll(target, 1.0f);
    ctrl.tick(0.5f);
    REQUIRE(ctrl.state() == AnimationController::State::Spinning);
    ctrl.tick(0.6f);  // total > 1.0s
    REQUIRE(ctrl.state() == AnimationController::State::Settled);
}

TEST_CASE("settled orientation matches target") {
    AnimationController ctrl;
    glm::quat target = glm::angleAxis(1.23f, glm::normalize(glm::vec3(1,1,0)));
    ctrl.roll(target, 0.5f);
    ctrl.tick(1.0f);  // advance well past duration
    glm::quat result = ctrl.currentOrientation();
    // dot product ~1 means same rotation
    float d = std::abs(glm::dot(result, target));
    REQUIRE_THAT(d, Catch::Matchers::WithinAbs(1.0f, 0.001f));
}
```

**Step 2: Run — expect FAIL**

**Step 3: Implement quaternion_utils.h/.cpp**

```cpp
// core/include/dice3d/animation/quaternion_utils.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace dice3d {

class QuaternionUtils {
public:
    // Shortest-path slerp (flips qB if dot(qA,qB) < 0)
    static glm::quat slerp(const glm::quat& a, const glm::quat& b, float t);

    // Easing functions (return value in [0,1] for t in [0,1])
    static float easeOutCubic(float t);
    static float easeOutQuint(float t);   // recommended for floating dice
    static float easeOutExpo(float t);
    static float easeInCubic(float t);    // used for correction blend
};

} // namespace dice3d
```

```cpp
// core/src/animation/quaternion_utils.cpp
#include "dice3d/animation/quaternion_utils.h"
#include <cmath>
using namespace dice3d;

glm::quat QuaternionUtils::slerp(const glm::quat& a, const glm::quat& b, float t) {
    glm::quat bb = b;
    if (glm::dot(a, b) < 0.0f) bb = -bb;  // shortest path
    return glm::normalize(glm::slerp(a, bb, t));
}

float QuaternionUtils::easeOutCubic(float t) { float x = 1-t; return 1 - x*x*x; }
float QuaternionUtils::easeOutQuint(float t) { float x = 1-t; return 1 - x*x*x*x*x; }
float QuaternionUtils::easeOutExpo(float t)  { return t==1?1:1-std::pow(2,-10*t); }
float QuaternionUtils::easeInCubic(float t)  { return t*t*t; }
```

**Step 4: Implement animation_controller.h/.cpp**

```cpp
// core/include/dice3d/animation/animation_controller.h
#pragma once
#include <glm/gtc/quaternion.hpp>
#include <random>

namespace dice3d {

class AnimationController {
public:
    enum class State { Idle, Spinning, Settled };

    AnimationController();

    // Begin a roll: spin for `duration` seconds and land on `target` orientation.
    void roll(const glm::quat& target, float duration);

    // Advance animation by dt seconds. Call every frame.
    void tick(float dt);

    State state() const { return _state; }
    glm::quat currentOrientation() const { return _orientation; }

private:
    State _state = State::Idle;
    glm::quat _orientation{1,0,0,0};

    // Roll state
    glm::quat _target{1,0,0,0};
    float _duration = 0.0f;
    float _elapsed  = 0.0f;

    // Tumble: angular velocity (radians/sec, multi-axis)
    glm::vec3 _angularVelocity{0,0,0};

    // Correction: q_corr = inverse(q_raw_end) * q_target
    // Blended in during the final ~30% of the animation
    glm::quat _qRawEnd{1,0,0,0};
    glm::quat _qCorr{1,0,0,0};

    std::mt19937 _rng;

    void initTumble();
};

} // namespace dice3d
```

```cpp
// core/src/animation/animation_controller.cpp
#include "dice3d/animation/animation_controller.h"
#include "dice3d/animation/quaternion_utils.h"
#include <glm/gtx/quaternion.hpp>
using namespace dice3d;

// Correction blend starts at this fraction of duration
static constexpr float kCorrectionStart = 0.70f;

AnimationController::AnimationController()
    : _rng(std::random_device{}()) {}

void AnimationController::roll(const glm::quat& target, float duration) {
    _target   = glm::normalize(target);
    _duration = duration;
    _elapsed  = 0.0f;
    _state    = State::Spinning;
    initTumble();
}

void AnimationController::initTumble() {
    // Random multi-axis angular velocity: 8-20 rad/s total speed
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speedDist(8.0f, 20.0f);
    _angularVelocity = glm::normalize(glm::vec3(
        axisDist(_rng), axisDist(_rng), axisDist(_rng)
    )) * speedDist(_rng);

    // Simulate where the tumble would end after `_duration` using
    // the initial velocity and easeOutQuint decay
    // We integrate numerically with ~60 steps
    glm::quat q = _orientation;
    float dt_sim = _duration / 60.0f;
    for (int i = 0; i < 60; i++) {
        float t = (float)i / 60.0f;
        float speedMul = 1.0f - QuaternionUtils::easeOutQuint(t);
        glm::vec3 w = _angularVelocity * speedMul;
        float angle = glm::length(w) * dt_sim;
        if (angle > 1e-6f) {
            glm::quat delta = glm::angleAxis(angle, glm::normalize(w));
            q = glm::normalize(delta * q);
        }
    }
    _qRawEnd = q;
    // correction: rotate qRawEnd into target
    _qCorr = glm::normalize(glm::inverse(_qRawEnd) * _target);
}

void AnimationController::tick(float dt) {
    if (_state != State::Spinning) return;

    _elapsed += dt;
    float t = std::min(_elapsed / _duration, 1.0f);

    // 1. Integrate angular velocity step (tumble)
    float speedMul = 1.0f - QuaternionUtils::easeOutQuint(t);
    glm::vec3 w = _angularVelocity * speedMul;
    float angle = glm::length(w) * dt;
    if (angle > 1e-6f) {
        glm::quat delta = glm::angleAxis(angle, glm::normalize(w));
        _orientation = glm::normalize(delta * _orientation);
    }

    // 2. Blend in correction during final 30%
    if (t > kCorrectionStart) {
        float corrT = (t - kCorrectionStart) / (1.0f - kCorrectionStart);
        float blend = QuaternionUtils::easeInCubic(corrT);
        // Apply partial correction: slerp between identity and full correction
        glm::quat partialCorr = QuaternionUtils::slerp(
            glm::quat(1,0,0,0), _qCorr, blend
        );
        _orientation = glm::normalize(_orientation * partialCorr);
    }

    if (t >= 1.0f) {
        _orientation = _target;
        _state = State::Settled;
    }
}
```

**Step 5: Run tests — expect pass**

```bash
cd build && ctest --output-on-failure -R test_animation
```

**Step 6: Commit**

```bash
git commit -m "feat: quaternion animation controller (tumble + correction slerp)"
```

---

## Task 6: Face mapper — face-to-camera orientation quaternions

**Files:**
- Create: `core/include/dice3d/rendering/face_mapper.h`
- Create: `core/src/rendering/face_mapper.cpp`
- Create: `core/tests/test_face_mapper.cpp`

The face mapper answers: "what quaternion do I need to set on the die so that face N points toward the camera?"

**Step 1: Write failing tests**

```cpp
// core/tests/test_face_mapper.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dice3d/geometry/polyhedra.h"
#include "dice3d/geometry/chamfer.h"
#include "dice3d/rendering/face_mapper.h"
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
        // Rotate the face normal by q
        glm::vec3 faceNormal = mapper.faceNormalForNumber(n);
        glm::vec3 rotated = q * faceNormal;
        // Should point toward camera: dot with -cameraDir should be ~1
        float d = glm::dot(rotated, -kCameraDir);
        REQUIRE_THAT(d, Catch::Matchers::WithinAbs(1.0f, 0.01f));
    }
}

TEST_CASE("face mapper d20 has entries for all 20 faces") {
    auto mesh = Chamfer::apply(Polyhedra::generate(20), 0.05f);
    FaceMapper mapper(mesh, kCameraDir, kCameraUp);
    for (int n = 1; n <= 20; n++)
        REQUIRE_NOTHROW(mapper.orientationForFace(n));
}
```

**Step 2: Implement face_mapper.h/.cpp**

```cpp
// core/include/dice3d/rendering/face_mapper.h
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
    glm::quat orientationForFace(int number) const;
    glm::vec3 faceNormalForNumber(int number) const;

private:
    std::unordered_map<int, glm::quat> _orientations;
    std::unordered_map<int, glm::vec3> _normals;

    // Build rotation that maps (faceNormal, faceUp) -> (-cameraForward, cameraUp)
    static glm::quat basisToCamera(
        const glm::vec3& faceNormal,
        const glm::vec3& faceUp,
        const glm::vec3& camForward,
        const glm::vec3& camUp
    );
};

} // namespace dice3d
```

```cpp
// core/src/rendering/face_mapper.cpp
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
    return _orientations.at(number);
}

glm::vec3 FaceMapper::faceNormalForNumber(int number) const {
    return _normals.at(number);
}

glm::quat FaceMapper::basisToCamera(
    const glm::vec3& fn, const glm::vec3& fu,
    const glm::vec3& cf, const glm::vec3& cu)
{
    // We want die rotation R such that:
    //   R * fn = -cf  (face normal points toward camera)
    //   R * fu = cu   (face up aligns with camera up)
    //
    // Construct orthonormal "from" basis (fn, fu, fn×fu)
    // and "to" basis (-cf, cu, -cf×cu), then build rotation matrix.
    glm::vec3 fn_norm = glm::normalize(fn);
    glm::vec3 fu_norm = glm::normalize(fu - fn_norm * glm::dot(fn_norm, fu));
    glm::vec3 fr_norm = glm::cross(fn_norm, fu_norm);

    glm::vec3 tn = glm::normalize(-cf);
    glm::vec3 tu = glm::normalize(cu - tn * glm::dot(tn, cu));
    glm::vec3 tr = glm::cross(tn, tu);

    // Rotation matrix: columns are to-basis vectors, rows are from-basis
    // R = [to_basis] * transpose([from_basis])
    glm::mat3 fromBasis(fn_norm, fu_norm, fr_norm);
    glm::mat3 toBasis(tn, tu, tr);
    glm::mat3 R = toBasis * glm::transpose(fromBasis);
    return glm::normalize(glm::quat_cast(R));
}
```

**Step 3: Run tests — expect pass**

```bash
cd build && ctest --output-on-failure -R test_face_mapper
```

**Step 4: Commit**

```bash
git commit -m "feat: face mapper — precomputed face-to-camera quaternions"
```

---

## Task 7: Filament renderer — scene setup

**Files:**
- Create: `core/include/dice3d/rendering/renderer.h`
- Create: `core/src/rendering/renderer.cpp`

This is the only task with no unit tests (requires a real Filament context). Instead, verify manually by building the iOS demo app in Task 11.

**Step 1: Implement renderer.h**

```cpp
// core/include/dice3d/rendering/renderer.h
#pragma once
#include <filament/Engine.h>
#include <filament/SwapChain.h>
#include <filament/Renderer.h>
#include <filament/View.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <utils/Entity.h>
#include "dice3d/geometry/mesh_builder.h"

namespace dice3d {

struct DieInstance {
    uint32_t handle;
    utils::Entity entity;
    glm::vec3 position;     // placement in scene (for multi-die layout)
    // Filament renderable components attached to entity
};

class Renderer {
public:
    // Takes ownership of nothing — caller owns the native window/layer pointer.
    explicit Renderer(filament::Engine::Backend backend = filament::Engine::Backend::DEFAULT);
    ~Renderer();

    // Surface lifecycle — call when native surface is created/destroyed/resized
    void attachSurface(void* nativeWindow, uint32_t width, uint32_t height);
    void detachSurface();
    void resize(uint32_t width, uint32_t height);

    // Die management
    uint32_t addDie(const GpuMesh& mesh, const glm::vec4& dieColor, bool whiteNumbers);
    void removeDie(uint32_t handle);
    void setDieTransform(uint32_t handle, const glm::quat& orientation, const glm::vec3& position);

    // Per-frame — must be called on the render thread
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

    std::unordered_map<uint32_t, DieInstance> _dice;
    uint32_t _nextHandle = 1;

    void setupCamera(uint32_t width, uint32_t height);
    void setupLighting();
};

} // namespace dice3d
```

**Step 2: Implement renderer.cpp — key patterns from research**

Critical rules from `docs/research/filament-cross-platform-dice.md`:
- Call `endFrame()` if and only if `beginFrame()` returns true
- Destroy swap chain before engine on teardown
- Transparent: use `CONFIG_TRANSPARENT` flag + `View::BlendMode::TRANSLUCENT`
- Resource destruction order: entities first, materials second, engine last

```cpp
// core/src/rendering/renderer.cpp
// Key snippet — renderFrame():
void Renderer::renderFrame() {
    if (!_swapChain) return;
    if (_renderer->beginFrame(_swapChain)) {
        _renderer->render(_view);
        _renderer->endFrame();
    }
}

// detachSurface():
void Renderer::detachSurface() {
    if (!_swapChain) return;
    _engine->flushAndWait();          // Filament Android UiHelper pattern
    _engine->destroy(_swapChain);
    _swapChain = nullptr;
}

// attachSurface():
void Renderer::attachSurface(void* nativeWindow, uint32_t w, uint32_t h) {
    detachSurface();
    _swapChain = _engine->createSwapChain(
        nativeWindow,
        filament::SwapChain::CONFIG_TRANSPARENT  // transparent background
    );
    _view->setBlendMode(filament::View::BlendMode::TRANSLUCENT);
    resize(w, h);
}
```

**Step 3: Commit**

```bash
git commit -m "feat: Filament renderer — scene, transparent swapchain, multi-die support"
```

---

## Task 8: DiceScene — top-level C++ object

**Files:**
- Create: `core/include/dice3d/dice_scene.h`
- Create: `core/src/dice_scene.cpp`
- Create: `core/include/dice3d/dice3d.h` (public C ABI)
- Create: `core/src/dice3d_impl.cpp`

**Step 1: Implement DiceScene (C++ object wrapping all components)**

```cpp
// core/include/dice3d/dice_scene.h
#pragma once
#include "rendering/renderer.h"
#include "animation/animation_controller.h"
#include "rendering/face_mapper.h"
#include "geometry/polyhedra.h"
#include "geometry/chamfer.h"
#include "geometry/mesh_builder.h"
#include <memory>

namespace dice3d {

struct DieConfig {
    int    sides;
    float  bevelFactor = 0.05f;
    glm::vec4 dieColor{0.8f, 0.1f, 0.1f, 1.0f};  // RGBA
    bool   whiteNumbers = true;
};

class DiceScene {
public:
    explicit DiceScene(filament::Engine::Backend backend = filament::Engine::Backend::DEFAULT);

    void attachSurface(void* nativeWindow, uint32_t w, uint32_t h);
    void detachSurface();
    void resize(uint32_t w, uint32_t h);

    // Returns opaque handle for the die
    uint32_t addDie(const DieConfig& config);
    void removeDie(uint32_t handle);

    // Roll: spin for `duration` seconds and land on `result`
    void roll(uint32_t handle, int result, float duration);
    void rollAll(const std::vector<std::pair<uint32_t, int>>& rolls, float duration);

    // Call every frame (from display link / Choreographer callback)
    void tick(float dt);
    void renderFrame();

private:
    struct Die {
        uint32_t renderHandle;
        std::unique_ptr<AnimationController> anim;
        std::unique_ptr<FaceMapper> faceMap;
        glm::vec3 position;
    };

    std::unique_ptr<Renderer> _renderer;
    std::unordered_map<uint32_t, Die> _dice;
    uint32_t _nextHandle = 1;

    void layoutDice();  // reposition dice when count changes
};

} // namespace dice3d
```

**Step 2: Implement C ABI (dice3d.h + dice3d_impl.cpp)**

The C ABI is what iOS (via Obj-C++) and Android (via JNI) actually call. All C++ stays hidden.

```c
// core/include/dice3d/dice3d.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Dice3DScene* Dice3DSceneRef;

Dice3DSceneRef dice3d_create(void);
void           dice3d_destroy(Dice3DSceneRef scene);

void dice3d_attach_surface(Dice3DSceneRef scene, void* nativeWindow,
                           uint32_t width, uint32_t height);
void dice3d_detach_surface(Dice3DSceneRef scene);
void dice3d_resize(Dice3DSceneRef scene, uint32_t width, uint32_t height);

uint32_t dice3d_add_die(Dice3DSceneRef scene, int sides,
                        float bevel,
                        float r, float g, float b, float a,
                        int whiteNumbers);
void dice3d_remove_die(Dice3DSceneRef scene, uint32_t handle);

void dice3d_roll(Dice3DSceneRef scene, uint32_t handle,
                 int result, float duration);

void dice3d_tick(Dice3DSceneRef scene, float dt);
void dice3d_render_frame(Dice3DSceneRef scene);

#ifdef __cplusplus
}
#endif
```

**Step 3: Commit**

```bash
git commit -m "feat: DiceScene top-level object and C ABI surface"
```

---

## Task 9: iOS platform adapter (Obj-C++)

**Files:**
- Create: `platform/ios/DiceRenderer.h`
- Create: `platform/ios/DiceRenderer.mm`
- Create: `platform/ios/DiceView.h`
- Create: `platform/ios/DiceView.mm`

**Rules from research (read before implementing):**
- Obj-C++ `.mm` file owns all Filament objects
- `CAMetalLayer*` passed to `createSwapChain` via `__bridge` — Filament does NOT take ownership
- Destroy swap chain before engine; call `flushAndWait()` first
- Metal shader warmup: call render once off-screen before showing view (avoids ~200ms stutter)
- Watch for drawable throttling — do not block render thread

**Step 1: Implement DiceRenderer.h (Obj-C++ class)**

```objc
// platform/ios/DiceRenderer.h
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#include "dice3d/dice3d.h"

@interface DiceRenderer : NSObject

- (instancetype)init;
- (void)attachLayer:(CAMetalLayer*)layer width:(uint32_t)w height:(uint32_t)h;
- (void)detachLayer;
- (void)resize:(uint32_t)w height:(uint32_t)h;

- (uint32_t)addDieWithSides:(int)sides
                      bevel:(float)bevel
                   dieColor:(UIColor*)color
               whiteNumbers:(BOOL)white;
- (void)removeDie:(uint32_t)handle;
- (void)rollDie:(uint32_t)handle result:(int)result duration:(float)duration;
- (void)tick:(float)dt;
- (void)renderFrame;

@end
```

**Step 2: Implement DiceRenderer.mm**

```objc
// platform/ios/DiceRenderer.mm
#import "DiceRenderer.h"

@implementation DiceRenderer {
    Dice3DSceneRef _scene;
}

- (instancetype)init {
    self = [super init];
    if (self) _scene = dice3d_create();
    return self;
}

- (void)dealloc {
    dice3d_detach_surface(_scene);
    dice3d_destroy(_scene);
}

- (void)attachLayer:(CAMetalLayer*)layer width:(uint32_t)w height:(uint32_t)h {
    // __bridge: no ownership transfer — Filament does not own the layer
    void* nativeWindow = (__bridge void*)layer;
    dice3d_attach_surface(_scene, nativeWindow, w, h);
}

- (void)detachLayer {
    dice3d_detach_surface(_scene);
}

// ... other methods forward directly to C ABI
@end
```

**Step 3: Implement DiceView.h/.mm (UIView subclass)**

```objc
// platform/ios/DiceView.h
#import <UIKit/UIKit.h>
#import "DiceRenderer.h"

@interface DiceView : UIView
@property (nonatomic, readonly) DiceRenderer* renderer;
- (void)startRenderLoop;
- (void)stopRenderLoop;
@end
```

```objc
// platform/ios/DiceView.mm
#import "DiceView.h"

@implementation DiceView {
    CADisplayLink* _displayLink;
    CFTimeInterval _lastTime;
}

+ (Class)layerClass { return [CAMetalLayer class]; }

- (void)startRenderLoop {
    _renderer = [[DiceRenderer alloc] init];
    CGSize s = self.bounds.size;
    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = NO;  // needed for transparent compositing

    [_renderer attachLayer:layer
                    width:(uint32_t)(s.width * self.contentScaleFactor)
                   height:(uint32_t)(s.height * self.contentScaleFactor)];

    _lastTime = CACurrentMediaTime();
    _displayLink = [CADisplayLink displayLinkWithTarget:self
                                             selector:@selector(_tick)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                      forMode:NSRunLoopCommonModes];
}

- (void)_tick {
    CFTimeInterval now = CACurrentMediaTime();
    float dt = (float)(now - _lastTime);
    _lastTime = now;
    [_renderer tick:dt];
    [_renderer renderFrame];
}

- (void)stopRenderLoop {
    [_displayLink invalidate];
    _displayLink = nil;
    [_renderer detachLayer];
}
@end
```

**Step 4: Commit**

```bash
git commit -m "feat: iOS Obj-C++ platform adapter (DiceView + DiceRenderer)"
```

---

## Task 10: Swift wrapper

**Files:**
- Create: `wrapper/swift/Sources/Dice3D/DiceView.swift`
- Create: `wrapper/swift/Sources/Dice3D/DiceController.swift`
- Create: `wrapper/swift/Package.swift`

The Swift wrapper is a thin convenience layer over the Obj-C++ API.

```swift
// wrapper/swift/Sources/Dice3D/DiceController.swift
import Foundation
import UIKit

@objc public class DiceController: NSObject {
    private let view: DiceView

    @objc public init(view: DiceView) {
        self.view = view
    }

    @objc public func addDie(sides: Int, color: UIColor, whiteNumbers: Bool) -> UInt32 {
        return view.renderer.addDie(
            withSides: Int32(sides),
            bevel: 0.05,
            dieColor: color,
            whiteNumbers: whiteNumbers
        )
    }

    @objc public func roll(handle: UInt32, result: Int, duration: Float) {
        view.renderer.rollDie(handle, result: Int32(result), duration: duration)
    }
}
```

**Step 5: Commit**

```bash
git commit -m "feat: Swift wrapper — DiceController and DiceView public API"
```

---

## Task 11: Android JNI adapter

**Files:**
- Create: `platform/android/dice3d_jni.cpp`
- Create: `platform/android/src/main/java/com/dice3d/DiceRenderer.kt`
- Create: `platform/android/src/main/java/com/dice3d/DiceView.kt`

**Rules from research:**
- `ANativeWindow_fromSurface` to get native window from Java `Surface`
- Release `ANativeWindow` ref after Filament creates swap chain
- Call `flushAndWait()` in `onDetachedFromSurface`
- Support both `SurfaceView` and `TextureView` for transparency
- Android backend: `DEFAULT` (allows Filament to pick Vulkan on API 28)

**Step 1: JNI bridge (dice3d_jni.cpp)**

```cpp
// platform/android/dice3d_jni.cpp
#include <jni.h>
#include <android/native_window_jni.h>
#include "dice3d/dice3d.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_dice3d_DiceRenderer_nativeCreate(JNIEnv*, jobject) {
    return (jlong)dice3d_create();
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeDestroy(JNIEnv*, jobject, jlong ptr) {
    dice3d_destroy((Dice3DSceneRef)ptr);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeAttachSurface(
    JNIEnv* env, jobject, jlong ptr, jobject surface,
    jint width, jint height)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
    dice3d_attach_surface((Dice3DSceneRef)ptr, win, width, height);
    ANativeWindow_release(win);  // release our ref; Filament holds its own if needed
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeDetachSurface(JNIEnv*, jobject, jlong ptr) {
    dice3d_detach_surface((Dice3DSceneRef)ptr);
}

JNIEXPORT jint JNICALL
Java_com_dice3d_DiceRenderer_nativeAddDie(
    JNIEnv*, jobject, jlong ptr, jint sides,
    jfloat bevel, jfloat r, jfloat g, jfloat b, jfloat a, jint white)
{
    return (jint)dice3d_add_die((Dice3DSceneRef)ptr, sides, bevel, r, g, b, a, white);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeRoll(
    JNIEnv*, jobject, jlong ptr, jint handle, jint result, jfloat duration)
{
    dice3d_roll((Dice3DSceneRef)ptr, handle, result, duration);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeTick(JNIEnv*, jobject, jlong ptr, jfloat dt) {
    dice3d_tick((Dice3DSceneRef)ptr, dt);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeRenderFrame(JNIEnv*, jobject, jlong ptr) {
    dice3d_render_frame((Dice3DSceneRef)ptr);
}

} // extern "C"
```

**Step 2: Kotlin wrapper**

```kotlin
// platform/android/src/main/java/com/dice3d/DiceRenderer.kt
package com.dice3d

import android.view.Surface

class DiceRenderer {
    private var nativePtr: Long = nativeCreate()

    fun attachSurface(surface: Surface, width: Int, height: Int) =
        nativeAttachSurface(nativePtr, surface, width, height)

    fun detachSurface() = nativeDetachSurface(nativePtr)

    fun addDie(sides: Int, bevel: Float = 0.05f,
               r: Float, g: Float, b: Float, a: Float = 1f,
               whiteNumbers: Boolean = true): Int =
        nativeAddDie(nativePtr, sides, bevel, r, g, b, a, if (whiteNumbers) 1 else 0)

    fun roll(handle: Int, result: Int, duration: Float) =
        nativeRoll(nativePtr, handle, result, duration)

    fun tick(dt: Float) = nativeTick(nativePtr, dt)
    fun renderFrame() = nativeRenderFrame(nativePtr)

    fun destroy() { nativeDestroy(nativePtr); nativePtr = 0 }

    companion object {
        init { System.loadLibrary("dice3d") }
        @JvmStatic private external fun nativeCreate(): Long
        @JvmStatic private external fun nativeDestroy(ptr: Long)
        @JvmStatic private external fun nativeAttachSurface(ptr: Long, surface: Surface, w: Int, h: Int)
        @JvmStatic private external fun nativeDetachSurface(ptr: Long)
        @JvmStatic private external fun nativeAddDie(ptr: Long, sides: Int, bevel: Float,
                                                      r: Float, g: Float, b: Float, a: Float, white: Int): Int
        @JvmStatic private external fun nativeRoll(ptr: Long, handle: Int, result: Int, duration: Float)
        @JvmStatic private external fun nativeTick(ptr: Long, dt: Float)
        @JvmStatic private external fun nativeRenderFrame(ptr: Long)
    }
}
```

**Step 3: Commit**

```bash
git commit -m "feat: Android JNI adapter and Kotlin DiceRenderer wrapper"
```

---

## Task 12: Build scripts — XCFramework (iOS) and AAR (Android)

**Files:**
- Create: `scripts/build_ios.sh`
- Create: `scripts/build_xcframework.sh`
- Create: `scripts/build_android.sh`

**Step 1: iOS build script**

```bash
#!/bin/bash
# scripts/build_ios.sh
set -e

FILAMENT_DIR="${FILAMENT_DIR:-$(pwd)/vendor/filament-ios}"
BUILD_DIR="$(pwd)/build-ios"

# Device (arm64)
cmake -B "$BUILD_DIR/device" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DDEPLOYMENT_TARGET=14.0 \
    -DFILAMENT_DIR="$FILAMENT_DIR" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/device" --config Release

# Simulator (arm64 + x86_64)
cmake -B "$BUILD_DIR/sim" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATORARM64 \
    -DDEPLOYMENT_TARGET=14.0 \
    -DFILAMENT_DIR="$FILAMENT_DIR" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/sim" --config Release
```

**Step 2: XCFramework packaging script**

```bash
#!/bin/bash
# scripts/build_xcframework.sh
set -e
xcodebuild -create-xcframework \
    -library build-ios/device/core/libdice3d.a \
    -headers core/include \
    -library build-ios/sim/core/libdice3d.a \
    -headers core/include \
    -output dist/Dice3D.xcframework
echo "XCFramework built at dist/Dice3D.xcframework"
```

**Step 3: Android build script**

```bash
#!/bin/bash
# scripts/build_android.sh
set -e
NDK="${ANDROID_NDK_ROOT:-$HOME/Library/Android/sdk/ndk/25.2.9519653}"
for ABI in arm64-v8a armeabi-v7a; do
    cmake -B "build-android/$ABI" \
        -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-28 \
        -DFILAMENT_DIR="${FILAMENT_DIR}" \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "build-android/$ABI" --config Release
done
# Copy .so files into AAR jniLibs structure
mkdir -p platform/android/src/main/jniLibs/{arm64-v8a,armeabi-v7a}
cp build-android/arm64-v8a/core/libdice3d.so platform/android/src/main/jniLibs/arm64-v8a/
cp build-android/armeabi-v7a/core/libdice3d.so platform/android/src/main/jniLibs/armeabi-v7a/
echo "Android .so files placed. Run gradle assembleRelease for AAR."
```

**Step 4: Commit**

```bash
git commit -m "chore: iOS XCFramework and Android AAR build scripts"
```

---

## Task 13: Texture atlas — pre-baked number assets

**Files:**
- Create: `core/assets/atlas/d4.png`, `d6.png`, `d8.png`, `d10.png`, `d12.png`, `d16.png`, `d20.png`, `d32.png`

Each atlas is a 512x512 PNG containing all face labels for that die type.

**Step 1: Atlas layout rules**
- Grid layout: ceil(sqrt(N)) columns × rows
- Each cell: power-of-2 size with 8px padding on each side
- Number rendered centered in cell, upright, in white on transparent background
- Separate white atlas per die (tinted to any color in shader)
- Generate using any tool (Photoshop, Sketch, code) — these are shipped assets

**Step 2: Shader uses atlas as alpha mask**

```glsl
// Filament material (.mat source, compiled to .filamat with matc):
material {
    name : "dice",
    shadingModel : lit,
    parameters : [
        { type : sampler2d, name : numberAtlas },
        { type : float4,    name : dieColor },
        { type : float4,    name : numberColor }
    ]
}
fragment {
    void material(inout MaterialInputs material) {
        prepareMaterial(material);
        float2 uv = getUV0();
        float alpha = texture(materialParams_numberAtlas, uv).r;
        // Blend number color over die color
        material.baseColor = mix(materialParams.dieColor,
                                 materialParams.numberColor,
                                 alpha);
    }
}
```

Compile: `matc -o core/assets/dice.filamat core/assets/dice.mat`

**Step 3: Commit**

```bash
git commit -m "feat: pre-baked texture atlas assets and Filament PBR material"
```

---

## Completion Checklist

Before declaring this plan done, verify:

- [ ] `ctest` passes all unit tests (geometry, chamfer, animation, face_mapper)
- [ ] iOS simulator build produces a `.a` and can be linked into a test iOS app
- [ ] Android ARM64 build produces `libdice3d.so`
- [ ] XCFramework contains both device and simulator slices
- [ ] A minimal iOS demo app shows a spinning d20 that lands on face 20
- [ ] A minimal Android demo app shows the same
- [ ] Chamfered edges are visually visible (compare bevel_factor=0.0 vs 0.05)
- [ ] Multiple dice on screen simultaneously work (try 2d6)
- [ ] Background is transparent (host UI shows through)
- [ ] `dice3d_detach_surface` + `dice3d_attach_surface` cycle does not crash (lifecycle test)
