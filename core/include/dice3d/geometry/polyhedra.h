#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace dice3d {

struct Face {
    std::vector<int> indices;   // vertex indices (3 = triangle, 4 = quad)
    glm::vec3 normal;           // outward unit normal
    glm::vec3 up;               // in-face "up" for text orientation
    int faceNumber;             // die face value (1-based); 0 = bevel face
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

    static void enforceOutwardNormals(PolyMesh& mesh);
    static glm::vec3 faceNormal(const PolyMesh& mesh, const Face& face);
    static glm::vec3 faceCentroid(const PolyMesh& mesh, const Face& face);
};

} // namespace dice3d
