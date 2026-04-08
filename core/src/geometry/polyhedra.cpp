#include "dice3d/geometry/polyhedra.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>

using namespace dice3d;

static constexpr float kPi = 3.14159265358979323846f;
static const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;  // golden ratio ≈ 1.618

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

PolyMesh Polyhedra::generate(int sides) {
    switch (sides) {
        case  4: return d4();
        case  6: return d6();
        case  8: return d8();
        case 10: return d10();
        case 12: return d12();
        case 16: return d16();
        case 20: return d20();
        case 32: return d32();
        default:
            throw std::invalid_argument("Unsupported die type: " + std::to_string(sides));
    }
}

// ---------------------------------------------------------------------------
// Utility: project world +Y onto face plane; fallback to +X when degenerate
// ---------------------------------------------------------------------------
static glm::vec3 upFromNormal(const glm::vec3& n) {
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 u = worldUp - n * glm::dot(n, worldUp);
    if (glm::length(u) < 1e-4f) {
        glm::vec3 fallback(1.0f, 0.0f, 0.0f);
        u = fallback - n * glm::dot(n, fallback);
    }
    return glm::normalize(u);
}

// ---------------------------------------------------------------------------
// d4 — tetrahedron: 4 vertices, 4 triangular faces
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d4() {
    PolyMesh m;
    m.vertices = {
        { 1.0f,  1.0f,  1.0f},
        { 1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
    };
    m.faces = {
        {{0, 1, 2}, {}, {}, 1},
        {{0, 2, 3}, {}, {}, 2},
        {{0, 3, 1}, {}, {}, 3},
        {{1, 3, 2}, {}, {}, 4},
    };
    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// d6 — cube: 8 vertices, 6 quad faces
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d6() {
    PolyMesh m;
    m.vertices = {
        {-1.0f, -1.0f, -1.0f},  // 0
        { 1.0f, -1.0f, -1.0f},  // 1
        { 1.0f,  1.0f, -1.0f},  // 2
        {-1.0f,  1.0f, -1.0f},  // 3
        {-1.0f, -1.0f,  1.0f},  // 4
        { 1.0f, -1.0f,  1.0f},  // 5
        { 1.0f,  1.0f,  1.0f},  // 6
        {-1.0f,  1.0f,  1.0f},  // 7
    };
    // Faces defined with CCW winding from the outside; enforceOutwardNormals
    // will correct any that are backwards.
    m.faces = {
        {{4, 5, 6, 7}, {}, {0.0f, 1.0f, 0.0f}, 1},  // +Z front
        {{1, 0, 3, 2}, {}, {0.0f, 1.0f, 0.0f}, 2},  // -Z back
        {{3, 7, 6, 2}, {}, {0.0f, 0.0f,-1.0f}, 3},  // +Y top
        {{0, 1, 5, 4}, {}, {0.0f, 0.0f, 1.0f}, 4},  // -Y bottom
        {{1, 2, 6, 5}, {}, {0.0f, 1.0f, 0.0f}, 5},  // +X right
        {{0, 4, 7, 3}, {}, {0.0f, 1.0f, 0.0f}, 6},  // -X left
    };
    enforceOutwardNormals(m);
    return m;
}

// ---------------------------------------------------------------------------
// d8 — octahedron: 6 vertices, 8 triangular faces
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d8() {
    PolyMesh m;
    m.vertices = {
        { 1.0f,  0.0f,  0.0f},  // 0  +X
        {-1.0f,  0.0f,  0.0f},  // 1  -X
        { 0.0f,  1.0f,  0.0f},  // 2  +Y
        { 0.0f, -1.0f,  0.0f},  // 3  -Y
        { 0.0f,  0.0f,  1.0f},  // 4  +Z
        { 0.0f,  0.0f, -1.0f},  // 5  -Z
    };
    m.faces = {
        {{0, 2, 4}, {}, {}, 1},
        {{0, 4, 3}, {}, {}, 2},
        {{0, 3, 5}, {}, {}, 3},
        {{0, 5, 2}, {}, {}, 4},
        {{1, 4, 2}, {}, {}, 5},
        {{1, 3, 4}, {}, {}, 6},
        {{1, 5, 3}, {}, {}, 7},
        {{1, 2, 5}, {}, {}, 8},
    };
    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// d10 — pentagonal trapezohedron (kite faces): 12 vertices, 10 quad faces
// McCooey canonical coordinates give short edges ≈ 0.618, long edges ≈ 1.618
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d10() {
    // McCooey constants (computed in double, stored as float)
    const float C0 = (float)((std::sqrt(5.0) - 1.0) / 4.0);  // ≈ 0.309
    const float C1 = (float)((1.0 + std::sqrt(5.0)) / 4.0);  // ≈ 0.809
    const float C2 = (float)((3.0 + std::sqrt(5.0)) / 4.0);  // ≈ 1.309

    PolyMesh m;
    m.vertices = {
        { 0.0f,  C0,  C1},  // V0
        { 0.0f,  C0, -C1},  // V1
        { 0.0f, -C0,  C1},  // V2
        { 0.0f, -C0, -C1},  // V3
        { 0.5f,  0.5f,  0.5f},  // V4
        { 0.5f,  0.5f, -0.5f},  // V5
        {-0.5f, -0.5f,  0.5f},  // V6
        {-0.5f, -0.5f, -0.5f},  // V7
        { C2, -C1,  0.0f},  // V8
        {-C2,  C1,  0.0f},  // V9
        { C0,  C1,  0.0f},  // V10
        {-C0, -C1,  0.0f},  // V11
    };

    // 10 kite faces: {apex, a, opposite, b}
    // Upper ring (apex = V8, pointing in +X direction)
    // Lower ring (apex = V9, pointing in -X direction)
    struct KiteFace { int apex, a, opp, b; int number; };
    const KiteFace kites[] = {
        {8, 2,  6, 11, 1}, {8, 11,  7,  3, 2}, {8,  3,  1,  5, 3},
        {8, 5, 10,  4, 4}, {8,  4,  0,  2, 5},
        {9, 0,  4, 10, 6}, {9, 10,  5,  1, 7}, {9,  1,  3,  7, 8},
        {9, 7, 11,  6, 9}, {9,  6,  2,  0,10},
    };

    for (auto& k : kites) {
        Face f;
        f.indices   = {k.apex, k.a, k.opp, k.b};
        f.faceNumber = k.number;
        m.faces.push_back(f);
    }

    enforceOutwardNormals(m);

    // Up vectors: project apex→opposite onto face plane
    for (auto& f : m.faces) {
        glm::vec3 apex = m.vertices[f.indices[0]];
        glm::vec3 opp  = m.vertices[f.indices[2]];
        glm::vec3 raw  = opp - apex;
        glm::vec3 n    = f.normal;
        glm::vec3 u    = raw - n * glm::dot(n, raw);
        if (glm::length(u) < 1e-4f) u = glm::vec3(0.0f, 1.0f, 0.0f);
        f.up = glm::normalize(u);
    }
    return m;
}

// ---------------------------------------------------------------------------
// d12 — dodecahedron: 20 vertices, 12 pentagonal faces
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d12() {
    const float p  = PHI;
    const float ip = 1.0f / PHI;

    PolyMesh m;
    m.vertices = {
        // (±1, ±1, ±1) × 8
        { 1.0f,  1.0f,  1.0f},  // 0
        { 1.0f,  1.0f, -1.0f},  // 1
        { 1.0f, -1.0f,  1.0f},  // 2
        { 1.0f, -1.0f, -1.0f},  // 3
        {-1.0f,  1.0f,  1.0f},  // 4
        {-1.0f,  1.0f, -1.0f},  // 5
        {-1.0f, -1.0f,  1.0f},  // 6
        {-1.0f, -1.0f, -1.0f},  // 7
        // (0, ±1/φ, ±φ) × 4
        { 0.0f,  ip,  p},  // 8
        { 0.0f,  ip, -p},  // 9
        { 0.0f, -ip,  p},  // 10
        { 0.0f, -ip, -p},  // 11
        // (±1/φ, ±φ, 0) × 4
        { ip,  p,  0.0f},  // 12
        {-ip,  p,  0.0f},  // 13
        { ip, -p,  0.0f},  // 14
        {-ip, -p,  0.0f},  // 15
        // (±φ, 0, ±1/φ) × 4
        { p,  0.0f,  ip},  // 16
        { p,  0.0f, -ip},  // 17
        {-p,  0.0f,  ip},  // 18
        {-p,  0.0f, -ip},  // 19
    };

    m.faces = {
        {{0,  8,  4, 13, 12}, {}, {}, 1},
        {{0, 12,  1, 17, 16}, {}, {}, 2},
        {{0, 16,  2, 10,  8}, {}, {}, 3},
        {{1, 12, 13,  5,  9}, {}, {}, 4},
        {{1,  9, 11,  3, 17}, {}, {}, 5},
        {{2, 16, 17,  3, 14}, {}, {}, 6},
        {{2, 14, 15,  6, 10}, {}, {}, 7},
        {{3, 11,  7, 15, 14}, {}, {}, 8},
        {{4,  8, 10,  6, 18}, {}, {}, 9},
        {{4, 18, 19,  5, 13}, {}, {},10},
        {{5, 19,  7, 11,  9}, {}, {},11},
        {{6, 15,  7, 19, 18}, {}, {},12},
    };

    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// d16 — octagonal dipyramid: 10 vertices (8 equator + 2 apices), 16 triangles
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d16() {
    PolyMesh m;
    const int  N = 8;
    const float R = 1.0f;   // equator radius
    const float H = 1.2f;   // apex height

    for (int k = 0; k < N; k++) {
        float angle = 2.0f * kPi * (float)k / (float)N;
        m.vertices.push_back({R * std::cos(angle), R * std::sin(angle), 0.0f});
    }
    m.vertices.push_back({0.0f, 0.0f,  H});  // index 8: top apex
    m.vertices.push_back({0.0f, 0.0f, -H});  // index 9: bottom apex

    const int top = N, bot = N + 1;
    for (int k = 0; k < N; k++) {
        int next = (k + 1) % N;
        m.faces.push_back({{top,    k, next}, {}, {}, k + 1});
        m.faces.push_back({{bot, next,    k}, {}, {}, k + 1 + N});
    }

    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// d20 — icosahedron: 12 vertices, 20 triangular faces
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d20() {
    const float t = PHI;

    PolyMesh m;
    m.vertices = {
        {-1.0f,  t,  0.0f},  // 0
        { 1.0f,  t,  0.0f},  // 1
        {-1.0f, -t,  0.0f},  // 2
        { 1.0f, -t,  0.0f},  // 3
        { 0.0f, -1.0f,  t},  // 4
        { 0.0f,  1.0f,  t},  // 5
        { 0.0f, -1.0f, -t},  // 6
        { 0.0f,  1.0f, -t},  // 7
        { t,  0.0f, -1.0f},  // 8
        { t,  0.0f,  1.0f},  // 9
        {-t,  0.0f, -1.0f},  // 10
        {-t,  0.0f,  1.0f},  // 11
    };

    m.faces = {
        // 5 faces around vertex 0
        {{ 0, 11,  5}, {}, {}, 1},
        {{ 0,  5,  1}, {}, {}, 2},
        {{ 0,  1,  7}, {}, {}, 3},
        {{ 0,  7, 10}, {}, {}, 4},
        {{ 0, 10, 11}, {}, {}, 5},
        // 5 adjacent faces
        {{ 1,  5,  9}, {}, {}, 6},
        {{ 5, 11,  4}, {}, {}, 7},
        {{11, 10,  2}, {}, {}, 8},
        {{10,  7,  6}, {}, {}, 9},
        {{ 7,  1,  8}, {}, {},10},
        // 5 faces around vertex 3
        {{ 3,  9,  4}, {}, {},11},
        {{ 3,  4,  2}, {}, {},12},
        {{ 3,  2,  6}, {}, {},13},
        {{ 3,  6,  8}, {}, {},14},
        {{ 3,  8,  9}, {}, {},15},
        // 5 adjacent faces
        {{ 4,  9,  5}, {}, {},16},
        {{ 2,  4, 11}, {}, {},17},
        {{ 6,  2, 10}, {}, {},18},
        {{ 8,  6,  7}, {}, {},19},
        {{ 9,  8,  1}, {}, {},20},
    };

    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// d32 — pentakis dodecahedron: 20 dodeca verts + 12 apex verts = 32, 60 triangles
// ---------------------------------------------------------------------------
PolyMesh Polyhedra::d32() {
    const float p  = PHI;
    const float ip = 1.0f / PHI;

    PolyMesh m;
    // 20 dodecahedron vertices (same coordinates as d12)
    m.vertices = {
        { 1.0f,  1.0f,  1.0f},  // 0
        { 1.0f,  1.0f, -1.0f},  // 1
        { 1.0f, -1.0f,  1.0f},  // 2
        { 1.0f, -1.0f, -1.0f},  // 3
        {-1.0f,  1.0f,  1.0f},  // 4
        {-1.0f,  1.0f, -1.0f},  // 5
        {-1.0f, -1.0f,  1.0f},  // 6
        {-1.0f, -1.0f, -1.0f},  // 7
        { 0.0f,  ip,  p},       // 8
        { 0.0f,  ip, -p},       // 9
        { 0.0f, -ip,  p},       // 10
        { 0.0f, -ip, -p},       // 11
        { ip,  p,  0.0f},       // 12
        {-ip,  p,  0.0f},       // 13
        { ip, -p,  0.0f},       // 14
        {-ip, -p,  0.0f},       // 15
        { p,  0.0f,  ip},       // 16
        { p,  0.0f, -ip},       // 17
        {-p,  0.0f,  ip},       // 18
        {-p,  0.0f, -ip},       // 19
    };

    // 12 pentagonal faces of the dodecahedron
    const std::vector<std::vector<int>> dodecaFaces = {
        { 0,  8,  4, 13, 12},
        { 0, 12,  1, 17, 16},
        { 0, 16,  2, 10,  8},
        { 1, 12, 13,  5,  9},
        { 1,  9, 11,  3, 17},
        { 2, 16, 17,  3, 14},
        { 2, 14, 15,  6, 10},
        { 3, 11,  7, 15, 14},
        { 4,  8, 10,  6, 18},
        { 4, 18, 19,  5, 13},
        { 5, 19,  7, 11,  9},
        { 6, 15,  7, 19, 18},
    };

    // Apex vertex for each pentagon: centroid pushed outward
    const float apexScale = 1.15f;
    for (auto& fi : dodecaFaces) {
        glm::vec3 centroid(0.0f);
        for (int i : fi) centroid += m.vertices[i];
        centroid /= (float)fi.size();
        m.vertices.push_back(glm::normalize(centroid) * apexScale);
    }

    // 5 triangles per pentagon: (apex, edge_a, edge_b)
    int faceNum = 1;
    for (int f = 0; f < 12; f++) {
        int apex       = 20 + f;
        const auto& fi = dodecaFaces[f];
        for (int i = 0; i < 5; i++) {
            int a = fi[i], b = fi[(i + 1) % 5];
            m.faces.push_back({{apex, a, b}, {}, {}, faceNum});
            if (++faceNum > 32) faceNum = 1;
        }
    }

    enforceOutwardNormals(m);
    for (auto& f : m.faces)
        f.up = upFromNormal(f.normal);
    return m;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

glm::vec3 Polyhedra::faceNormal(const PolyMesh& mesh, const Face& face) {
    const auto& v   = mesh.vertices;
    const auto& idx = face.indices;
    glm::vec3 a = v[idx[1]] - v[idx[0]];
    glm::vec3 b = v[idx[2]] - v[idx[0]];
    return glm::normalize(glm::cross(a, b));
}

glm::vec3 Polyhedra::faceCentroid(const PolyMesh& mesh, const Face& face) {
    glm::vec3 c(0.0f);
    for (int i : face.indices) c += mesh.vertices[i];
    return c / (float)face.indices.size();
}

void Polyhedra::enforceOutwardNormals(PolyMesh& mesh) {
    // Precondition: mesh is origin-centered (centroid test assumes origin-relative faces)
    for (auto& f : mesh.faces) {
        f.normal          = faceNormal(mesh, f);
        glm::vec3 centroid = faceCentroid(mesh, f);
        if (glm::dot(f.normal, centroid) < 0.0f) {
            std::reverse(f.indices.begin(), f.indices.end());
            f.normal = -f.normal;
        }
    }
}
