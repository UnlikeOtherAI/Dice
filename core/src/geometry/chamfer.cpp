#include "dice3d/geometry/chamfer.h"
#include <map>
#include <algorithm>
#include <cmath>
using namespace dice3d;

PolyMesh Chamfer::apply(const PolyMesh& input, float bevel_factor) {
    if (bevel_factor <= 0.0f) return input;

    PolyMesh result;

    // Phase 1: Create inset face for each original face.
    // Each original vertex is moved toward the face centroid by bevel_factor.
    // Inset vertices are appended to result.vertices in face-local order.
    std::vector<std::vector<int>> insetFaceVerts(input.faces.size());

    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        const Face& face = input.faces[fi];
        glm::vec3 centroid(0);
        for (int vi : face.indices) centroid += input.vertices[vi];
        centroid /= (float)face.indices.size();

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

    // Phase 2: For each shared edge between two faces, create a bevel quad.
    // Edge key: ordered pair (min vertex index, max vertex index).
    // Value: list of (faceIndex, localA, localB) where edge goes localA->localB.
    struct EdgeRef { int faceIdx, localA, localB; };
    std::map<std::pair<int,int>, std::vector<EdgeRef>> edgeMap;

    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        const auto& idx = input.faces[fi].indices;
        for (int k = 0; k < (int)idx.size(); k++) {
            int a = idx[k], b = idx[(k+1) % (int)idx.size()];
            auto key = std::make_pair(std::min(a,b), std::max(a,b));
            edgeMap[key].push_back({fi, k, (k+1) % (int)idx.size()});
        }
    }

    for (auto& [edge, refs] : edgeMap) {
        if (refs.size() != 2) continue;  // boundary edge, skip
        auto& r0 = refs[0];
        auto& r1 = refs[1];
        int v0a = insetFaceVerts[r0.faceIdx][r0.localA];
        int v0b = insetFaceVerts[r0.faceIdx][r0.localB];
        int v1a = insetFaceVerts[r1.faceIdx][r1.localA];
        int v1b = insetFaceVerts[r1.faceIdx][r1.localB];
        // Bevel quad connects the two adjacent inset faces across their shared edge.
        // Winding: v0a, v0b from face 0 (outward); v1a, v1b from face 1 (reversed).
        Face bevel;
        bevel.indices = {v0a, v0b, v1a, v1b};
        bevel.faceNumber = 0;
        bevel.normal = glm::normalize(
            input.faces[r0.faceIdx].normal + input.faces[r1.faceIdx].normal
        );
        bevel.up = glm::vec3(0, 1, 0);
        result.faces.push_back(bevel);
    }

    // Phase 3: For each original vertex, create a corner cap polygon.
    // Collect all inset vertices derived from that original vertex, sort them
    // angularly around the vertex position, and add a polygon cap.
    std::map<int, std::vector<int>> vertexToInset;
    for (int fi = 0; fi < (int)input.faces.size(); fi++) {
        const auto& orig_idx = input.faces[fi].indices;
        for (int k = 0; k < (int)orig_idx.size(); k++) {
            vertexToInset[orig_idx[k]].push_back(insetFaceVerts[fi][k]);
        }
    }

    for (auto& [origVert, insetVerts] : vertexToInset) {
        if ((int)insetVerts.size() < 3) continue;

        glm::vec3 center = input.vertices[origVert];
        glm::vec3 normal = glm::normalize(center);  // outward: assumes origin-centered mesh
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
        cap.normal = normal;
        cap.faceNumber = 0;
        cap.up = glm::vec3(0, 1, 0);
        result.faces.push_back(cap);
    }

    return result;
}
