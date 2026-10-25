#!/usr/bin/env bash
# Re-export the STL pieces used by the 3D viewer after changing the model.
# Usage: OPENSCAD=/path/to/openscad ./export-viewer-models.sh   (default: openscad on PATH)
set -euo pipefail

cd "$(dirname "$0")"
OPENSCAD="${OPENSCAD:-openscad}"
mkdir -p models

for piece in bottom_shell top_shell caps tact_switches battery charger esp32 gyro power_switch; do
  echo "exporting $piece"
  "$OPENSCAD" --backend=manifold --export-format=binstl -D "piece=\"$piece\"" \
    -o "models/$piece.stl" ../enclosure-viewer-pieces.scad
done
