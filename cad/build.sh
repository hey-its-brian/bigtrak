#!/usr/bin/env bash
# Render every printable part to STL, plus PNG previews. Output goes to
# cad/out/ (not checked in). Needs OpenSCAD on the PATH.
#
#   cad/build.sh            everything
#   cad/build.sh drive_pod  just the parts whose name starts with that
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
OUT=out
mkdir -p "$OUT"
FILTER="${1:-}"

# name | source file | -D overrides
PARTS=(
  "drive_pod|drive_pod.scad|part=\"pod\""
  "drive_wheel|drive_wheel.scad|"
  "cyd_bezel|cyd_bezel.scad|part=\"bezel\""
  "cyd_back|cyd_back.scad|"
  "fit_tests|fit_tests.scad|part=\"all\""
  "fit_shaft|fit_tests.scad|part=\"shaft\""
  "fit_face|fit_tests.scad|part=\"face\""
  "fit_cyd|fit_tests.scad|part=\"cyd\""
  "fit_groove|fit_tests.scad|part=\"groove\""
)

# Previews only (not printable)
PREVIEWS=(
  "drive_pod_assembly|drive_pod.scad|part=\"assembly\""
  "cyd_assembly|cyd_bezel.scad|part=\"assembly\""
)

for entry in "${PARTS[@]}"; do
  IFS='|' read -r name src def <<<"$entry"
  [[ -n "$FILTER" && "$name" != "$FILTER"* ]] && continue
  echo "== $name"
  d=(); [[ -n "$def" ]] && d=(-D "$def")
  openscad -q ${d[@]+"${d[@]}"} -o "$OUT/$name.stl" "$src"
  openscad -q ${d[@]+"${d[@]}"} --render --autocenter --viewall --imgsize=1000,750 \
    --colorscheme=Tomorrow -o "$OUT/$name.png" "$src"
done

for entry in "${PREVIEWS[@]}"; do
  IFS='|' read -r name src def <<<"$entry"
  [[ -n "$FILTER" && "$name" != "$FILTER"* ]] && continue
  echo "== $name (preview)"
  d=(); [[ -n "$def" ]] && d=(-D "$def")
  openscad -q ${d[@]+"${d[@]}"} --autocenter --viewall --imgsize=1000,750 \
    --colorscheme=Tomorrow -o "$OUT/$name.png" "$src"
done
