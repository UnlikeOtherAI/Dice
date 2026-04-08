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
    static constexpr float kCellPadding = 0.18f;

    // --- UV atlas layout ---
    std::set<int> faceNums;
    for (auto& f : mesh.faces) if (f.faceNumber > 0) faceNums.insert(f.faceNumber);
    int N = (int)faceNums.size();
    int cols = N > 0 ? (int)std::ceil(std::sqrt((float)N)) : 1;
    int rows = std::max((N + cols - 1) / cols, 1);  // guard: N=0 must not produce rows=0
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
        float maxAbsU = 0.0f;
        float maxAbsV = 0.0f;
        for (int k = 0; k < vertCount; k++) {
            glm::vec3 d = mesh.vertices[face.indices[k]] - centroid;
            localCoords[k] = {glm::dot(d, right), glm::dot(d, up)};
            maxAbsU = std::max(maxAbsU, std::abs(localCoords[k].x));
            maxAbsV = std::max(maxAbsV, std::abs(localCoords[k].y));
        }
        float halfExtent = std::max(std::max(maxAbsU, maxAbsV), 1e-6f);

        bool hasNumberCell = face.faceNumber > 0 && faceCell.count(face.faceNumber);
        glm::vec4 cell = hasNumberCell ? faceCell[face.faceNumber] : glm::vec4(0.0f);
        float insetU0 = cell.x + cellW * kCellPadding;
        float insetV0 = cell.y + cellH * kCellPadding;
        float insetU1 = cell.z - cellW * kCellPadding;
        float insetV1 = cell.w - cellH * kCellPadding;

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
            // Keep the label centered and preserve aspect ratio instead of
            // stretching it to each face's full bounding box.
            float au;
            float av;
            if (hasNumberCell) {
                float u = 0.5f - (localCoords[k].x / (2.0f * halfExtent));
                float v = 0.5f + (localCoords[k].y / (2.0f * halfExtent));
                au = insetU0 + u * (insetU1 - insetU0);
                av = insetV0 + v * (insetV1 - insetV0);
            } else {
                // Non-number faces should always sample transparent atlas space.
                au = 0.0f;
                av = 0.0f;
            }
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
