// JGA25-370 gearmotor: a reference model for assembly previews, plus the
// mounting-face cutouts that every part bolting to the motor shares.
//
// Motor coordinates: the gearbox face is at x = 0, the shaft points +x,
// the body runs toward -x. The two M3 holes lie on the y axis.

include <../params.scad>

$fn = $preview ? 48 : 96;

// The D-shaft profile, centred, `extra` added all round for clearance.
module d_shaft_2d(extra = 0) {
  r = motor_shaft_d / 2 + extra;
  flat_from_centre = motor_shaft_flat - motor_shaft_d / 2 + extra;
  intersection() {
    circle(r = r);
    translate([-r, -r]) square([r + flat_from_centre, 2 * r]);
  }
}

// Reference motor, for assembly previews only.
module jga25_motor() {
  color("silver") {
    // gearbox
    rotate([0, -90, 0]) cylinder(d = motor_gearbox_d, h = motor_gearbox_len);
    // motor can and encoder
    translate([-motor_gearbox_len, 0, 0])
      rotate([0, -90, 0]) cylinder(d = motor_can_d, h = motor_rear_len);
  }
  color("gold") {
    // boss and shaft
    rotate([0, 90, 0]) cylinder(d = motor_boss_d, h = motor_boss_h);
    translate([motor_boss_h, 0, 0])
      rotate([0, 90, 0]) linear_extrude(motor_shaft_len) rotate(90) d_shaft_2d();
  }
  color("white")
    translate([-motor_total_len + 2, -motor_conn_w / 2, motor_can_d / 2 - 2])
      cube([motor_conn_len, motor_conn_w, motor_conn_overhang + 2]);
}

// Cutouts for a wall of thickness `wall` whose inner face sits against the
// gearbox face (x = 0) and whose outer face is at x = wall. Subtract this.
module jga25_face_cutouts(wall) {
  eps = 0.01;
  // boss
  translate([-eps, 0, 0]) rotate([0, 90, 0])
    cylinder(d = motor_boss_d + 2 * clearance + 0.6, h = wall + 2 * eps);
  // M3 clearance holes with a counterbore on the outside for the heads
  for (y = [-1, 1] * motor_hole_spacing / 2) {
    translate([-eps, y, 0]) rotate([0, 90, 0])
      cylinder(d = m3_clear_d, h = wall + 2 * eps);
  }
  // pockets for the proud gearbox screw heads, on the inner face
  for (z = [-1, 1] * motor_hole_spacing / 2) {
    translate([-eps, 0, z]) rotate([0, 90, 0])
      cylinder(d = motor_screwhead_d + 0.6, h = motor_screwhead_h + 0.4);
  }
}
