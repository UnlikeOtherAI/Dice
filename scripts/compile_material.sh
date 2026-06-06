#!/bin/bash
set -e
# Requires matc from Filament tools (in $FILAMENT_DIR/tools/matc or PATH).
# -a all => emit GLSL (OpenGL), SPIR-V (Vulkan) AND Metal shaders, so the same
# .filamat works on the Android Vulkan backend (emulator + device) and iOS Metal.
# Without Vulkan/SPIR-V, Material::build() panics under the Vulkan backend.
MATC="${MATC:-matc}"
"$MATC" -a all -o core/assets/dice.filamat core/assets/dice.mat
echo "Material compiled (all backends) to core/assets/dice.filamat"
