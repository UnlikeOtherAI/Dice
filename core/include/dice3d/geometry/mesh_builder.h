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
    // faceNumber -> UV cell rect (u0, v0, u1, v1) in atlas
    // faceNumber 0 = bevel/cap face, no label cell
    std::vector<std::pair<int, glm::vec4>> faceAtlasCells;
};

class MeshBuilder {
public:
    // Build GPU mesh from chamfered PolyMesh.
    // atlasSize: texture atlas width/height in pixels (square)
    static GpuMesh build(const PolyMesh& mesh, int atlasSize = 512);
};

} // namespace dice3d
