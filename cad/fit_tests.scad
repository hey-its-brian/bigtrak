// Small, fast prints that check the critical fits before committing to the
// big parts. Each takes a few minutes and a few grams.
//
//   shaft    three D-bores at different clearances. Push each onto the motor
//            shaft; put the best one's value in params.scad shaft_clearance.
//            One dot = tightest, three = loosest.
//   face     the motor mounting face: boss hole, M3 holes, pockets for the
//            proud gearbox screws. Bolt it to a motor; it should sit flat.
//   cyd      the board's hole pattern on a thin frame with pegs. Drop the
//            board on: the pegs should go through all four holes.
//   groove   a slice of the drive wheel rim with one O-ring groove. Roll a
//            #232 O-ring on: snug, seated, and not trying to jump off.
//   all      all four on one plate.

include <params.scad>
use <lib/jga25.scad>
use <lib/cyd.scad>

part = "all";

$fn = $preview ? 48 : 96;

shaft_variants = [shaft_clearance - 0.05, shaft_clearance, shaft_clearance + 0.06];

module shaft_coupon() {
  difference() {
    translate([-18, -7, 0]) cube([36, 14, 10]);
    for (i = [0 : 2]) {
      translate([(i - 1) * 12, 0, 1.2]) linear_extrude(10)
        d_shaft_2d(shaft_variants[i]);
      // dots on the side so the variants can't be mixed up
      for (d = [0 : i])
        translate([(i - 1) * 12 + (d - i / 2) * 2.4, -7.01, 7]) rotate([-90, 0, 0])
          cylinder(d = 1.4, h = 0.6);
    }
  }
}

module face_coupon() {
  t = 3;
  difference() {
    translate([0, 0, 0]) linear_extrude(t) offset(3) offset(-3)
      square([32, 32], center = true);
    // jga25_face_cutouts works along +x; turn it to cut along +z, with the
    // motor side down on the bed so the screw-head pockets are on the
    // underside, like the real wall.
    rotate([0, -90, 0]) jga25_face_cutouts(t);
  }
}

module cyd_frame() {
  t = 1.2;
  difference() {
    linear_extrude(t) cyd_outline_2d();
    translate([0, 0, -1]) linear_extrude(t + 2) offset(-7) square([cyd_pcb_l, cyd_pcb_w], center = true);
  }
  for (h = cyd_holes())
    translate([h[0], h[1], 0]) {
      cylinder(d = 7, h = t);
      cylinder(d = cyd_hole_d - 2 * clearance, h = t + 3);
    }
}

module groove_ring() {
  include_d = oring_id * 1.02;
  depth = oring_section * 0.7;
  r = oring_section / 2 + 0.15;
  h = oring_section + 3;
  difference() {
    cylinder(d = include_d + 2 * depth, h = h);
    translate([0, 0, -1]) cylinder(d = include_d - 6, h = h + 2);
    translate([0, 0, h / 2]) rotate_extrude() translate([include_d / 2 + r, 0]) circle(r = r);
  }
}

if (part == "shaft") shaft_coupon();
if (part == "face") face_coupon();
if (part == "cyd") cyd_frame();
if (part == "groove") groove_ring();
if (part == "all") {
  translate([-60, 50, 0]) shaft_coupon();
  translate([-60, 15, 0]) face_coupon();
  translate([30, 50, 0]) groove_ring();
  translate([0, -45, 0]) cyd_frame();
}
