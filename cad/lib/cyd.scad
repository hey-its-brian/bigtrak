// ESP32-3248S035C reference geometry shared by the bezel and back cover.
//
// Board coordinates: front view, landscape, USB on the left. Origin at the
// board centre, x along the length, y along the width (+y = top edge),
// z = 0 at the PCB's front face, display toward +z.

include <../params.scad>

function cyd_holes() = [
  for (sx = [-1, 1], sy = [-1, 1])
    [sx * (cyd_pcb_l / 2 - cyd_hole_inset), sy * (cyd_pcb_w / 2 - cyd_hole_inset)]
];

// Active area centre relative to the board centre.
function cyd_aa_centre() = [-cyd_pcb_l / 2 + cyd_aa_x + cyd_aa_l / 2, 0];

module cyd_outline_2d(extra = 0) {
  offset(r = cyd_pcb_r + extra) offset(delta = -cyd_pcb_r)
    square([cyd_pcb_l, cyd_pcb_w], center = true);
}

// Reference board, for previews only.
module cyd_board() {
  color("darkgreen") translate([0, 0, -cyd_pcb_t])
    linear_extrude(cyd_pcb_t) difference() {
      cyd_outline_2d();
      for (h = cyd_holes()) translate(h) circle(d = cyd_hole_d);
    }
  color("black") linear_extrude(cyd_front_stack)
    square([cyd_lcd_l, cyd_pcb_w], center = true);
  color("steelblue") translate([cyd_aa_centre()[0], 0, cyd_front_stack])
    linear_extrude(0.1) square([cyd_aa_l, cyd_aa_w], center = true);
  color("dimgray") translate([0, 0, -cyd_pcb_t - cyd_rear_h])
    linear_extrude(cyd_rear_h) offset(-6) square([cyd_pcb_l, cyd_pcb_w], center = true);
}
