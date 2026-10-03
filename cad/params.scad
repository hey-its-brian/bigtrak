// Shared dimensions for every printed part. All units mm.
//
// Three kinds of numbers live here:
//   DRAWING  from a manufacturer drawing; trust it, but check against the
//            fit tests before printing the big parts
//   MEASURE  a placeholder until it's measured on the real tank or part;
//            the parts render with these values but WILL NOT FIT until
//            they're replaced
//   TUNE     printer and material tolerances; adjust after the fit tests
//
// Sources for the DRAWING values are listed in cad/README.md.

// ============================================================== TUNE =====

// Extra radius on holes and bores that a part slides over (per side).
clearance = 0.20;
// Bore tolerance for the D-shaft press/slip fit. fit_tests.scad prints
// three variants; put the winner here.
shaft_clearance = 0.12;
// Pilot hole for M3 self-tapping into PETG/PLA, and the hole for an M3
// heat-set insert (the common 5.7mm long, 4.0mm OD kind).
m3_tap_d = 2.6;
m3_insert_d = 4.0;
m3_clear_d = 3.4;
m3_head_d = 6.0;
m3_nut_flats = 5.5;
m3_nut_h = 2.4;

// ======================================================= JGA25-370 =======
// 25GA-370 / JGA25-370 12V gearmotor with hall encoder (NFP outline drawing).

motor_gearbox_d = 25.0;          // DRAWING
motor_can_d = 24.4;              // DRAWING (drawing labels it 25; sellers 24.4)
motor_body_clear_d = 25.6;       // bore for the whole body, gearbox to encoder
// Gearbox length depends on the ratio: 17 (4.4, 9.6), 19 (21), 21 (35, 46),
// 23 (78, 103), 25 (171, 226), 27 (377, 500). Varies ~2mm by vendor.
motor_gearbox_len = 21;          // MEASURE on your motor
motor_rear_len = 40.0;           // DRAWING: gearbox joint to back of encoder magnet
motor_boss_d = 7.0;              // DRAWING: raised bushing around the shaft
motor_boss_h = 2.5;              // DRAWING
motor_shaft_d = 4.0;             // DRAWING
motor_shaft_flat = 3.5;          // DRAWING: across the D flat
motor_shaft_len = 9.6;           // DRAWING: exposed past the boss
motor_shaft_flat_len = 8.0;      // DRAWING
motor_hole_spacing = 17.0;       // DRAWING: 2x M3 tapped, on a line through the shaft
// The gearbox assembly screws stand proud of the face, on the axis at 90
// degrees to the M3 holes. The mount needs pockets for them.
motor_screwhead_d = 5.0;         // DRAWING
motor_screwhead_h = 1.2;         // MEASURE (not on the drawing; 1.2 is a guess)
// Encoder connector: 6-pin, ~9 wide, sticks out radially past the body at
// the back. The cradle leaves a slot for it.
motor_conn_w = 10.0;             // DRAWING ~9, plus room
motor_conn_overhang = 5.0;       // MEASURE: how far past the body radius it sticks out
motor_conn_len = 8.0;            // MEASURE: length of the connector along the motor

motor_total_len = motor_gearbox_len + motor_rear_len;  // face to back of encoder

// ====================================================== Big Trak =========
// Original 1979 Milton Bradley Big Trak. The middle pair of wheels is the
// driven pair; front and rear are idlers.

// Driven-wheel tyre: the original uses #232 O-rings (2-3/4" ID, 1/8" section).
oring_id = 69.85;                // DRAWING (AS568 #232)
oring_section = 3.18;            // DRAWING
oring_count = 2;                 // MEASURE: how many O-rings per middle wheel
wheel_width = 40;                // MEASURE: middle wheel width (2010 copy is ~45)

// Axle line height above the floor of the drive pod (the surface the
// original gearbox mounted to).
axle_height = 24;                // MEASURE

// Distance between the inner faces of the two middle wheels. The two motors
// sit back to back in this space.
wheel_inner_gap = 150;           // MEASURE

// Original gearbox mounting: 4 screws (#4 thread-forming, 3/8"). Positions
// are [x, y] relative to the axle centre (x across the tank, y forward).
gearbox_screws = [[-55, 20], [55, 20], [-55, -20], [55, -20]];  // MEASURE
gearbox_screw_clear_d = 3.2;     // #4 screw clearance

// Footprint of the drive pod base plate, centred on the axle.
pod_plate_len = 60;              // MEASURE: room front-to-back around the axle

// =============================================== ESP32-3248S035C =========
// Sunton 3.5" CYD, capacitive (Jingcai dimension drawing + JC3248A035N).

cyd_pcb_l = 101.5;               // DRAWING
cyd_pcb_w = 54.9;                // DRAWING
cyd_pcb_t = 1.6;                 // MEASURE (assumed standard)
cyd_pcb_r = 3.5;                 // MEASURE (corner radius, scaled from photo)
cyd_hole_inset = 3.5;            // DRAWING: hole centres from each edge
cyd_hole_d = 3.2;                // DRAWING
cyd_lcd_l = 85.5;                // DRAWING: module outline
cyd_aa_l = 73.44;                // DRAWING: active area
cyd_aa_w = 48.96;                // DRAWING
// Active area offset along the length depends on which end the LCD flex is
// at: 11.7 or 16.4 from the left (USB-side) PCB edge. Look at the board:
// the flex is the end with the wider black border.
cyd_aa_x = 11.7;                 // MEASURE: left PCB edge to start of active area
// Glass front to PCB front face (LCD module 2.5 + touch glass ~1).
cyd_front_stack = 3.6;           // MEASURE
// Tallest part on the back of the PCB (ESP32 module, USB, JST headers).
cyd_rear_h = 4.5;                // MEASURE

// Connector positions, from the front, landscape, USB on the left. Offsets
// are along the edge, ~2mm accuracy (scaled from a photo).
cyd_usb_from_top = 17;           // MEASURE: left edge, USB centre from the top edge
cyd_usb_w = 12;                  // opening for a USB-C or micro-USB plug body
cyd_usb_h = 7;
cyd_p1_from_bottom = 14;         // MEASURE: left edge, P1 (power) centre
cyd_p3_from_right = 16;          // MEASURE: top edge, P3 (UART to drive board)
cyd_p4_from_right = 43;          // MEASURE: bottom edge, P4 (speaker)
cyd_buttons_from_right = [17, 21];  // MEASURE: bottom edge, RESET and BOOT

// ===================================================== keypad recess =====
// The opening in the top shell where the membrane keypad sat.

recess_l = 130;                  // MEASURE: opening length
recess_w = 80;                   // MEASURE: opening width
recess_r = 5;                    // MEASURE: corner radius
shell_t = 2.5;                   // MEASURE: shell wall thickness at the opening
recess_depth = 12;               // MEASURE: floor of the recess (or open)
// Screw holes through the bezel lip into the shell, [x, y] from the recess
// centre. Empty means no screws (tape or friction fit).
bezel_screws = [];               // MEASURE: original keypad fixings, if any
