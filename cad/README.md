# cad/: printed parts

Parametric OpenSCAD for the parts the rebuild needs: a drive pod that
replaces the original gearbox, printed drive wheels for the new motors, and
a bezel and back cover that mount the CYD in the keypad recess.

**Status: first draft, not yet fitted.** Motor and CYD dimensions come from
manufacturer drawings. The tank's own dimensions are placeholders marked
`MEASURE` in [params.scad](params.scad), and the drive pod and bezel won't
fit until those are filled in. The fit tests and the drive wheels can be
printed now.

```bash
cad/build.sh              # renders every part to cad/out/*.stl and *.png
cad/build.sh drive_pod    # just one
```

Open the `.scad` files in OpenSCAD to tweak interactively; every file
pulls its numbers from `params.scad`.

## Parts

| Part | File | Qty | Material | Orientation |
|---|---|---|---|---|
| Fit tests | `fit_tests.scad` | 1 set | any | as rendered |
| Drive pod | `drive_pod.scad` | 1 | PETG | base plate down |
| Drive wheel | `drive_wheel.scad` | 2 | PETG | hubcap face down |
| CYD bezel | `cyd_bezel.scad` | 1 | PLA or PETG, colour to taste | display face down |
| CYD back cover | `cyd_back.scad` | 1 | PLA | floor down |

None need supports. 0.2mm layers, 4 walls, 30% gyroid is plenty for all of
them; use 5 walls on the drive wheels and pod for the screw bosses. The
pod and wheels are PETG because a stalled motor gets warm and PLA creeps
under sustained load.

### Fit tests first

`fit_tests.scad` prints four small coupons in a few minutes each. Print them
before anything big:

| Coupon | Check | If it's off |
|---|---|---|
| Shaft (3 bores, 1 to 3 dots) | Push each onto the motor shaft. Pick the one that's snug with no wobble. | Set `shaft_clearance` to its value (dots: -0.05, +0, +0.06 from the current setting) |
| Motor face | Bolt it to the motor face with 2 M3x6. It must sit flat. | Rocking means the screw-head pockets are short: raise `motor_screwhead_h` |
| CYD frame | Drop the board on; all four pegs through the holes | Adjust `cyd_hole_inset` |
| Groove ring | Roll a #232 O-ring on; seated, not jumping off | Tune the groove in `drive_wheel.scad` |

### Drive pod

The original Big Trak gearbox is a sealed unit that holds both motors and
the middle (driven) wheels, fixed by four screws. The pod bolts to those
same four points. Each JGA25-370 sits in a cradle with its gearbox face
against an end wall (2x M3x6 into the motor face), held down with two zip
ties, and drives its middle wheel directly: no gears, no axle.

The pod pushes the motors as far out as the wheels allow and reports the
fit when rendered:

```
ECHO: "drive_pod: 148mm wide, gap between encoders 20mm, shaft into wheel 8.1mm"
```

If the wheels are closer together than two motors (about 136mm of motor
and wall), rendering stops with an error saying how much room it needs. A
shorter gearbox ratio buys back 2 to 8mm per side.

### Drive wheels

New wheels rather than adapters: nothing published documents the
original's drive hub, and printing the wheel keeps the originals untouched.
The rim takes the same #232 O-rings the original used (2-3/4" ID, 1/8"
section), stretched about 2% onto the grooves. The hub has a D-bore for
the 4mm shaft, an M3 set screw onto the flat, and a captive nut slot. The
outer face has a shallow 40mm recess for a hubcap sticker.

Tyre diameter is about 77mm, roughly 243mm per revolution. With a 46:1
motor that's about 2024 encoder ticks per rev, or roughly 2,700 ticks per
13" Big Trak unit: a reasonable first `cal unit` value before you measure
properly (see the main README).

### CYD bezel and back cover

The bezel's lip rests on the shell around the keypad opening; a skirt
locates it in the opening. The display shows through a bevelled window
sized to the active area. Four standoffs carry the board.

The back cover is a tray under the board. One set of four M3 screws (the
render prints the length, M3x15 by default) runs through the cover's
spacer tubes and the board into the bezel standoffs. It has openings for USB
(reflashing with the cover on), P1 power, P3 to the drive board, P4 to the
speaker, and the RESET/BOOT buttons.

How the bezel holds to the shell depends on how the original keypad was
held: list screw positions in `bezel_screws` for countersunk holes in the
lip, or leave it empty and use VHB tape.

## Hardware

| Item | Qty | For |
|---|---|---|
| M3x6 pan head | 4 | motors to pod walls |
| M3x5 set screw + M3 nut | 2 | wheels to shafts (use threadlocker) |
| Zip ties, 4mm wide | 4 | motors into cradles |
| #4 x 3/8" thread-forming (originals) | 4 | pod to chassis |
| #232 O-ring (NBR, 2-3/4" ID x 1/8") | 2 per wheel | tyres |
| M3x15 pan or button head | 4 | CYD back cover + board to bezel |

## Measure on the tank

In priority order. Calipers where it matters.

1. `wheel_inner_gap`: distance between the inner faces of the two middle wheels.
2. `axle_height`: axle centre above the surface the gearbox mounts to.
3. `gearbox_screws`: the four gearbox screw positions relative to the axle centre.
4. `pod_plate_len`: room front to back around the axle for the base plate.
5. `wheel_width` and `oring_count`: middle wheel width, and how many O-rings it wears.
6. Keypad recess: `recess_l`, `recess_w`, `recess_r`, `shell_t`, and how the keypad was held.
7. On the motors: `motor_gearbox_len` (depends on the ratio), `motor_screwhead_h`, and the encoder connector (`motor_conn_*`).
8. On the CYD: `cyd_aa_x` (which end the LCD flex is on), `cyd_front_stack`, `cyd_rear_h`, and the connector positions.

## Sources

- JGA25-370 with encoder: NFP outline drawing via
  [Makers Electronics](https://makerselectronics.com/product/dc-motor-ga25-370-with-encoder-4-4kg-130rpm-12v-with-bracket/),
  ratio and length table from [NFP](https://nfpmotor.com/25mm-metal-gear-motor-model-nfp-jga25-370-en).
- ESP32-3248S035: manufacturer dimension drawing and JC3248A035N LCD
  datasheet in [Jane-DIYmall/ESP32-3248S035](https://github.com/Jane-DIYmall/ESP32-3248S035).
- Big Trak drivetrain (middle wheels driven, #232 O-rings, 4-screw gearbox):
  [Robot Room](https://www.robotroom.com/BigTrak.html),
  [petervis](https://www.petervis.com/gallery/Toys_and_Games/bigtrak/bigtrak.html).

### Prior art worth knowing about

- codemonky's [Open Truck BigTrak](https://www.thingiverse.com/thing:5364605)
  (CC BY) models the original parts, including the driven wheel and lower
  body. If the printed wheels here ever need to look more original, that's
  a legitimate remix base.
- Kyle Delaney's [Big Trak page](https://kyles-lab.com/robots/bigtrak) has a
  rear wheel peg and a battery cover (no license stated; ask before remixing).
- Front axle replacements:
  [Gadgeteering](https://www.thingiverse.com/thing:3114277) (CC BY-NC),
  [Davidreeb](https://www.printables.com/model/1221671-front-axle-for-big-trak) (CC BY).
