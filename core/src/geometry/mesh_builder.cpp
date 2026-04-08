#define GLM_ENABLE_EXPERIMENTAL
#include "dice3d/geometry/mesh_builder.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <set>
#include <cmath>

using namespace dice3d;

GpuMesh MeshBuilder::build(const PolyMesh& mesh, int /*atlasSize*/) {
    GpuMesh result;

    // --- UV atlas layout ---
    std::set<int> faceNums;
    for (auto& f : mesh.faces) if (f.faceNumber > 0) faceNums.insert(f.faceNumber);
    int N = (int)faceNums.size();
    int cols = N > 0 ? (int)std::ceil(std::sqrt((float)N)) : 1;
    int rows = (N + cols - 1) / cols;
    float cellW = 1.0f / (float)cols;
    float cellH = 1.0f / (float)rows;
    std::map<int, glm::vec4> faceCell;  // faceNumber -> (u0, v0, u1, v1)
    {
        int i = 0;
        for (int fn : faceNums) {
            int col = i % cols, row = i / cols;
            float u0 = col * cellW, v0 = row * cellH;
            faceCell[fn] = {u0, v0, u0 + cellW, v0 + cellH};
            result.faceAtlasCells.push_back({fn, faceCell[fn]});
            i++;
        }
    }

    // --- Per-face vertex expansion + triangulation ---
    for (auto& face : mesh.faces) {
        const int vertCount = (int)face.indices.size();
        if (vertCount < 3) continue;

        glm::vec3 normal = face.normal;
        glm::vec3 up = face.up;
        // Ensure up is orthogonal to normal
        up = up - normal * glm::dot(normal, up);
        float upLen = glm::length(up);
        if (upLen < 1e-5f) {
            // Fallback: pick any perpendicular
            glm::vec3 arb = std::abs(normal.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
            up = arb - normal * glm::dot(normal, arb);
        }
        up = glm::normalize(up);
        glm::vec3 right = glm::normalize(glm::cross(up, normal));

        // Compute face centroid
        glm::vec3 centroid(0);
        for (int vi : face.indices) centroid += mesh.vertices[vi];
        centroid /= (float)vertCount;

        // Project face verts onto local 2D plane to compute UVs
        std::vector<glm::vec2> localCoords(vertCount);
        float minU = 1e9f, maxU = -1e9f, minV = 1e9f, maxV = -1e9f;
        for (int k = 0; k < vertCount; k++) {
            glm::vec3 d = mesh.vertices[face.indices[k]] - centroid;
            localCoords[k] = {glm::dot(d, right), glm::dot(d, up)};
            minU = std::min(minU, localCoords[k].x);
            maxU = std::max(maxU, localCoords[k].x);
            minV = std::min(minV, localCoords[k].y);
            maxV = std::max(maxV, localCoords[k].y);
        }
        float rangeU = std::max(maxU - minU, 1e-6f);
        float rangeV = std::max(maxV - minV, 1e-6f);

        // Get atlas cell for this face (bevel/cap faces use a corner sliver)
        glm::vec4 cell = {0.0f, 0.0f, cellW, cellH};  // default for bevel faces
        if (face.faceNumber > 0 && faceCell.count(face.faceNumber))
            cell = faceCell[face.faceNumber];

        // Tangent quaternion (same for all verts on this face)
        // Columns: T=right, B=up, N=normal
        glm::mat3 tbn(right, up, normal);
        glm::quat q = glm::normalize(glm::quat_cast(tbn));
        if (q.w < 0) q = -q;
        glm::vec4 tangentQ(q.x, q.y, q.z, q.w);

        // Emit expanded vertices (face-local, no sharing)
        uint32_t baseIdx = (uint32_t)result.positions.size();
        for (int k = 0; k < vertCount; k++) {
            result.positions.push_back(mesh.vertices[face.indices[k]]);
            result.normals.push_back(normal);
            result.tangents.push_back(tangentQ);
            // UV: normalize local coord to [0,1] then map into atlas cell
            float u = (localCoords[k].x - minU) / rangeU;
            float v = (localCoords[k].y - minV) / rangeV;
            float au = cell.x + u * (cell.z - cell.x);
            float av = cell.y + v * (cell.w - cell.y);
            result.uvs.push_back({au, av});
        }

        // Fan triangulation from vertex 0
        for (int k = 1; k < vertCount - 1; k++) {
            result.indices.push_back(baseIdx);
            result.indices.push_back(baseIdx + k);
            result.indices.push_back(baseIdx + k + 1);
        }
    }

    return result;
}
