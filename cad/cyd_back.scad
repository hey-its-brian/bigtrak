// CYD back cover: a shallow tray over the back of the board, inside the
// tank. Its four spacer tubes press on the board's mounting holes, and the
// same four M3 screws run through cover, spacer and board into the bezel
// standoffs, so one set of screws holds everything.
//
// Openings: USB on the left edge (reflashing), P1 power on the left edge,
// P3 UART on the top edge, P4 speaker and the RESET/BOOT buttons on the
// bottom edge. Print floor down, no supports.
//
// Board coordinates as in lib/cyd.scad (front view, USB left, +y top), but
// z = 0 is the board's back face and the cover hangs below it.

include <params.scad>
use <lib/cyd.scad>

$fn = $preview ? 32 : 64;

wall = 2.0;
floor_t = 2.0;
gap = 0.5;                       // between the board edge and the wall
depth = cyd_rear_h + 1.5;        // board back face to floor top
spacer_d = 7;
notch_w = 11;                    // cable/connector notches
vent_slots = 5;

outer_l = cyd_pcb_l + 2 * (gap + wall);
outer_w = cyd_pcb_w + 2 * (gap + wall);

// What screw to buy: floor + spacer + board + ~5mm into the standoff.
screw_len = floor_t + depth + cyd_pcb_t + 5;
echo(str("cyd_back: use M3 x ", ceil(screw_len), " (pan or button head)"));

module rounded_rect(l, w, r) {
  offset(r) offset(-r) square([l, w], center = true);
}

// A notch down from the rim (board side) through the wall at `pos` along
// the given edge. Edges: "left", "right", "top", "bottom".
module notch(edge, pos, w = notch_w, h = depth) {
  cut = wall + gap + 2;
  if (edge == "left")
    translate([-outer_l / 2 - 1, pos - w / 2, -h]) cube([cut, w, h + 1]);
  if (edge == "top")
    translate([pos - w / 2, outer_w / 2 - cut + 1, -h]) cube([w, cut, h + 1]);
  if (edge == "bottom")
    translate([pos - w / 2, -outer_w / 2 - 1, -h]) cube([w, cut, h + 1]);
}

module cover() {
  half_l = cyd_pcb_l / 2;
  half_w = cyd_pcb_w / 2;
  difference() {
    union() {
      // floor
      translate([0, 0, -depth - floor_t]) linear_extrude(floor_t)
        rounded_rect(outer_l, outer_w, cyd_pcb_r + gap + wall);
      // walls up to the board's back plane
      translate([0, 0, -depth - 0.01]) linear_extrude(depth + 0.01)
        difference() {
          rounded_rect(outer_l, outer_w, cyd_pcb_r + gap + wall);
          rounded_rect(outer_l - 2 * wall, outer_w - 2 * wall, cyd_pcb_r + gap);
        }
      // spacer tubes up to the board
      for (h = cyd_holes())
        translate([h[0], h[1], -depth - 0.01]) cylinder(d = spacer_d, h = depth + 0.01);
    }

    // screw holes, counterbored from below for the heads
    for (h = cyd_holes()) {
      translate([h[0], h[1], -depth - floor_t - 1]) cylinder(d = m3_clear_d, h = depth + floor_t + 2);
      translate([h[0], h[1], -depth - floor_t - 1]) cylinder(d = m3_head_d + 0.6, h = 1 + 1.0);
    }

    // USB: a full-height window in the left wall, so a cable plugs in with
    // the cover on
    translate([-outer_l / 2 - 1, half_w - cyd_usb_from_top - cyd_usb_w / 2, -depth - 0.01])
      cube([wall + gap + 2, cyd_usb_w, depth + 1]);
    notch("left", -half_w + cyd_p1_from_bottom);           // P1 power
    notch("top", half_l - cyd_p3_from_right);               // P3 UART
    notch("bottom", half_l - cyd_p4_from_right);            // P4 speaker
    // RESET/BOOT: one wide notch so a fingernail or pick reaches both
    notch("bottom", half_l - (cyd_buttons_from_right[0] + cyd_buttons_from_right[1]) / 2,
          w = abs(cyd_buttons_from_right[1] - cyd_buttons_from_right[0]) + 8);

    // vents over the ESP32 module
    for (i = [0 : vent_slots - 1])
      translate([-20 + i * 8, 0, -depth - floor_t - 1])
        linear_extrude(floor_t + 2) square([3, cyd_pcb_w * 0.5], center = true);
  }
}

// Printed floor down.
translate([0, 0, depth + floor_t]) cover();
