#!/bin/bash
set -e
# Requires matc from Filament tools (in $FILAMENT_DIR/tools/matc or PATH)
MATC="${MATC:-matc}"
"$MATC" -o core/assets/dice.filamat core/assets/dice.mat
echo "Material compiled to core/assets/dice.filamat"
