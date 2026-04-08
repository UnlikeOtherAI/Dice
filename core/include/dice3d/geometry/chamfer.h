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
