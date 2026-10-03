// Drive wheel: a printed replacement for the Big Trak's middle wheels, with
// the hub bored for the JGA25-370's 4mm D-shaft and grooves for #232
// O-rings, the tyres the original used. The original wheels stay untouched.
//
// Print outer face down (the hubcap side), no supports. One left, one right:
// they're identical, just flipped on the tank.
//
// Retention: an M3 set screw onto the shaft flat, with a captive nut. Use
// threadlocker; a set screw on a flat backs out under vibration.

include <params.scad>
use <lib/jga25.scad>

$fn = $preview ? 64 : 160;

// Stretch the O-rings ~2% onto the groove so they don't creep off.
groove_root_d = oring_id * 1.02;
groove_depth = oring_section * 0.7;
groove_r = oring_section / 2 + 0.15;
rim_d = groove_root_d + 2 * groove_depth;
rim_t = 3;                       // radial thickness under the grooves
web_t = 3.5;
hub_d = 16;
hubcap_d = 40;                   // shallow recess for a hubcap sticker
hubcap_depth = 0.6;
lightening_holes = 6;
rib_t = 2.4;                     // radial ribs from hub to rim, stiffen the hub
rib_h_frac = 0.6;                // ribs run this fraction of the width up from the web

// The shaft is exposed past the drive pod wall by this much, minus the gap
// to the wheel; that's how deep it goes into the hub.
shaft_into_wheel = motor_boss_h + motor_shaft_len - 3.0 - 1.0;
bore_depth = shaft_into_wheel + 0.6;
setscrew_z = wheel_width - shaft_into_wheel / 2;

tyre_d = groove_root_d + 2 * oring_section * 0.95;
echo(str("drive_wheel: tyre ~", tyre_d, "mm, ", PI * tyre_d,
         "mm per rev, shaft engagement ", shaft_into_wheel, "mm"));
assert(shaft_into_wheel >= 6, "shaft engagement under 6mm; check pod wall/gap");

module grooves() {
  for (i = [0 : oring_count - 1]) {
    z = wheel_width * (i + 1) / (oring_count + 1);
    translate([0, 0, z]) rotate_extrude()
      translate([groove_root_d / 2 + groove_r, 0]) circle(r = groove_r);
  }
}

module wheel() {
  difference() {
    union() {
      // rim
      difference() {
        cylinder(d = rim_d, h = wheel_width);
        translate([0, 0, -1]) cylinder(d = groove_root_d - 2 * rim_t, h = wheel_width + 2);
      }
      // web, near the outer face
      cylinder(d = groove_root_d - rim_t, h = web_t);
      // hub, full width
      cylinder(d = hub_d, h = wheel_width);
      // ribs between the lightening holes
      for (a = [180 / lightening_holes : 360 / lightening_holes : 359])
        rotate(a) translate([0, -rib_t / 2, 0])
          cube([groove_root_d / 2 - rim_t + 0.5, rib_t, wheel_width * rib_h_frac]);
    }

    grooves();

    // lightening holes in the web
    for (a = [0 : 360 / lightening_holes : 359])
      rotate(a) translate([(hub_d / 2 + groove_root_d / 2 - rim_t) / 2, 0, -1])
        cylinder(d = (groove_root_d / 2 - rim_t - hub_d / 2) * 0.7, h = web_t + 2);

    // hubcap sticker recess on the outer face
    translate([0, 0, -0.01]) cylinder(d = hubcap_d, h = hubcap_depth);

    // D bore from the inner face
    translate([0, 0, wheel_width - bore_depth])
      linear_extrude(bore_depth + 0.01) d_shaft_2d(shaft_clearance);
    // lead-in chamfer
    translate([0, 0, wheel_width - 0.8])
      cylinder(d1 = motor_shaft_d, d2 = motor_shaft_d + 1.6, h = 0.81);

    // set screw, radial, aimed at the flat (+x side of the bore)
    translate([0, 0, setscrew_z]) rotate([0, 90, 0])
      cylinder(d = m3_clear_d, h = hub_d);
    // captive nut, slid in from the inner face
    translate([motor_shaft_d / 2 + 1.6, -m3_nut_flats / 2 - 0.15, setscrew_z - m3_nut_flats / 2 - 0.2])
      cube([m3_nut_h + 0.3, m3_nut_flats + 0.3, wheel_width]);
  }
}

wheel();
