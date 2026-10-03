// Drive pod: replaces the original Big Trak gearbox. Bolts to the same four
// screw points and holds two JGA25-370 gearmotors back to back on the axle
// line, each driving one middle wheel directly.
//
// Print flat on the base plate, no supports. PETG preferred (motor torque
// and the warmth of a stalled motor will creep PLA over time).
//
// Coordinates: x across the tank (+x = right), y forward, z up. The axle is
// on the y = 0 line at z = axle_height. z = 0 is the underside of the plate.

include <params.scad>
use <lib/jga25.scad>

// Set by the build script; "pod" for the printable part, "assembly" for a
// preview with motors and wheels.
part = "pod";

$fn = $preview ? 48 : 96;

plate_t = 5;
wall_t = 3.0;           // motor face wall; M3x6 screws give ~3mm of thread
wheel_gap = 1.0;        // air between the wall and the wheel's inner face
cradle_w = motor_body_clear_d + 6;
cradle_len = motor_gearbox_len + 18;  // gearbox plus the front of the can
// The wall overhangs the cradle by this much each side, room for real gussets.
wall_half_w = cradle_w / 2 + 5;
ziptie_w = 4.5;
ziptie_t = 2.2;
screw_pad_d = 10;
screw_head_d = 6.8;     // counterbore under the plate for the #4 heads
screw_head_h = 2.5;

// Each motor's gearbox face sits just inside its wall, as far out as the
// wheels allow, so the shaft reaches the wheel with nothing in between.
face_x = wheel_inner_gap / 2 - wheel_gap - wall_t;
encoder_back_x = face_x - motor_total_len;
pod_half_x = face_x + wall_t;

motor_bottom_z = axle_height - motor_body_clear_d / 2;

assert(encoder_back_x >= 4,
       str("Motors don't fit between the wheels: they need ",
           2 * (motor_total_len + wall_t + wheel_gap) + 8,
           "mm, wheel_inner_gap is ", wheel_inner_gap));
assert(motor_bottom_z >= plate_t + 1,
       str("axle_height ", axle_height, " is too low for a ",
           motor_body_clear_d, "mm motor on a ", plate_t, "mm plate"));

echo(str("drive_pod: ", 2 * pod_half_x, "mm wide, gap between encoders ",
         2 * encoder_back_x, "mm, shaft into wheel ",
         motor_boss_h + motor_shaft_len - wall_t - wheel_gap, "mm"));

module rounded_square(size, r) {
  offset(r) offset(-r) square(size, center = true);
}

module base_plate() {
  difference() {
    hull() {
      linear_extrude(plate_t)
        rounded_square([2 * pod_half_x, max(pod_plate_len, 2 * wall_half_w)], 4);
      for (p = gearbox_screws)
        translate([p[0], p[1], 0]) cylinder(d = screw_pad_d, h = plate_t);
    }
    for (p = gearbox_screws) {
      translate([p[0], p[1], -1]) cylinder(d = gearbox_screw_clear_d, h = plate_t + 2);
      translate([p[0], p[1], -1]) cylinder(d = screw_head_d, h = screw_head_h + 1);
    }
  }
}

// One motor's wall and cradle, for the right-hand motor (shaft toward +x).
module motor_mount() {
  difference() {
    union() {
      // face wall, rounded over the top of the motor
      translate([face_x, 0, 0]) hull() {
        translate([0, -wall_half_w, 0]) cube([wall_t, 2 * wall_half_w, 1]);
        translate([0, 0, axle_height]) rotate([0, 90, 0])
          cylinder(r = wall_half_w, h = wall_t);
      }
      // cradle: a block under the motor, bored out to the axle line
      translate([face_x - cradle_len, -cradle_w / 2, 0])
        cube([cradle_len, cradle_w, axle_height]);
      // gussets tying the wall to the plate beside the cradle
      for (s = [-1, 1])
        translate([face_x, s * (cradle_w / 2 + (wall_half_w - cradle_w / 2) / 2), 0])
          rotate([90, 0, 0])
            linear_extrude(wall_half_w - cradle_w / 2, center = true)
              polygon([[0, 0], [-12, 0], [0, axle_height * 0.8]]);
    }

    // motor body
    translate([face_x + 0.01, 0, axle_height]) rotate([0, -90, 0])
      cylinder(d = motor_body_clear_d, h = cradle_len + 1);

    // face holes, boss, screw-head pockets
    translate([face_x, 0, axle_height]) jga25_face_cutouts(wall_t);

    // zip-tie slots under the motor: one at the gearbox, one at the can
    for (x = [face_x - motor_gearbox_len / 2, face_x - cradle_len + 6])
      translate([x - ziptie_w / 2, -cradle_w, motor_bottom_z - ziptie_t - 1.2])
        cube([ziptie_w, 2 * cradle_w, ziptie_t]);
  }
}

module drive_pod() {
  base_plate();
  motor_mount();
  mirror([1, 0, 0]) motor_mount();
}

module assembly() {
  drive_pod();
  for (s = [1, -1])
    mirror([s < 0 ? 1 : 0, 0, 0]) {
      translate([face_x, 0, axle_height]) jga25_motor();
      %translate([wheel_inner_gap / 2, 0, axle_height]) rotate([0, 90, 0])
        cylinder(d = oring_id + 2 * oring_section, h = wheel_width);
    }
}

if (part == "pod") drive_pod();
if (part == "assembly") assembly();
