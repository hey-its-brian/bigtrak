// CYD bezel: drops into the keypad recess in the top shell, frames the
// 3.5" display, and carries the board on four standoffs. The lip rests on
// the shell; the skirt locates it in the opening.
//
// The board screws to the standoffs from behind, then cyd_back.scad covers
// the back. Print face down (lip on the bed) for the cleanest visible
// surface; no supports needed.
//
// Coordinates: z = 0 is the top surface of the shell, +z up out of the tank.
// x along the recess length, y across it. The board sits centred.

include <params.scad>
use <lib/cyd.scad>

// "bezel" for the printable part (flipped face down), "assembly" for a
// preview in place with the board.
part = "bezel";

$fn = $preview ? 32 : 64;

lip_w = 4;              // how far the lip overlaps the shell around the opening
lip_t = 2.0;            // also the thickness of the frame over the display
fit = 0.4;              // gap between the skirt and the opening, per side
skirt_t = 1.8;
skirt_depth = shell_t + 2;
glass_gap = 0.2;        // air between the frame and the touch glass
window_margin = 0.6;    // window is this much bigger than the active area
window_bevel = 1.2;
standoff_d = 7;         // max ~8 before it hits the LCD module

// Glass front sits just under the frame; the board's front face is below it.
pcb_front_z = -glass_gap - cyd_front_stack;
standoff_len = -pcb_front_z;

opening_l = recess_l - 2 * fit;
opening_w = recess_w - 2 * fit;

assert(opening_l - 2 * skirt_t >= cyd_pcb_l + 0.6 && opening_w - 2 * skirt_t >= cyd_pcb_w + 0.6,
       str("The ", cyd_pcb_l, " x ", cyd_pcb_w, " board doesn't fit inside a ",
           recess_l, " x ", recess_w, " recess with ", skirt_t, "mm skirt walls"));

module rounded_rect(l, w, r) {
  offset(r) offset(-r) square([l, w], center = true);
}

module window() {
  c = cyd_aa_centre();
  l = cyd_aa_l + 2 * window_margin;
  w = cyd_aa_w + 2 * window_margin;
  translate([c[0], c[1], -1]) linear_extrude(lip_t + 2) square([l, w], center = true);
  // 45 degree bevel on the visible face
  translate([c[0], c[1], lip_t - window_bevel])
    hull() {
      linear_extrude(0.01) square([l, w], center = true);
      translate([0, 0, window_bevel + 0.01])
        linear_extrude(0.01) square([l + 2 * window_bevel, w + 2 * window_bevel], center = true);
    }
}

module bezel() {
  difference() {
    union() {
      // lip and display frame
      linear_extrude(lip_t)
        rounded_rect(recess_l + 2 * lip_w, recess_w + 2 * lip_w, recess_r + lip_w);
      // skirt into the opening
      translate([0, 0, -skirt_depth]) linear_extrude(skirt_depth + 0.01)
        difference() {
          rounded_rect(opening_l, opening_w, max(recess_r - fit, 0.5));
          rounded_rect(opening_l - 2 * skirt_t, opening_w - 2 * skirt_t,
                       max(recess_r - fit - skirt_t, 0.5));
        }
      // standoffs for the board
      for (h = cyd_holes())
        translate([h[0], h[1], pcb_front_z]) cylinder(d = standoff_d, h = standoff_len + 0.01);
    }

    window();

    // M3 self-tapping holes up through the standoffs into the frame, stopping
    // short of the visible face. Too short for heat-set inserts.
    for (h = cyd_holes())
      translate([h[0], h[1], pcb_front_z - 0.01])
        cylinder(d = m3_tap_d, h = standoff_len + lip_t - 0.8);

    // optional screws through the lip into the shell, countersunk
    for (s = bezel_screws)
      translate([s[0], s[1], -1]) {
        cylinder(d = 2.9, h = lip_t + 2);
        translate([0, 0, 1 + lip_t - 1.4]) cylinder(d1 = 2.9, d2 = 5.6, h = 1.41);
      }
  }
}

if (part == "bezel") rotate([180, 0, 0]) translate([0, 0, -lip_t]) bezel();
if (part == "assembly") {
  bezel();
  translate([0, 0, pcb_front_z]) cyd_board();
}
