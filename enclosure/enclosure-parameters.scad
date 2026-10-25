// AirPresenter enclosure - every dimension in one place (mm).
// Axes: x = width (0 at the centre line), y = length (0 = rear end with the charging
// port, case_len = front tip that points at the screen), z = height (0 = bottom face).
//
// SELLER LISTINGS DISAGREE BY 1-3 mm FOR THESE MODULES. Measure your parts with a
// caliper, enter the numbers in the "MEASURE" block, re-export, then print the fit
// test coupon (part = "fit_test") before the full case.

$fn = 48;

// ---------------- MEASURE: your components ----------------
// Board: [pcb width, pcb length (without USB), pcb thickness,
//         USB-C width, USB-C height off the PCB, USB-C sticks out past the PCB edge,
//         tallest thing on the face that points UP in the case,
//         flipped: 0 = component side up, 1 = pin side up (USB-C hangs under the PCB)]
// ESP32: pins/pads are on the bottom side, so it goes in upside down; the "up" height
// is solder joints / trimmed pins (cut pre-soldered header pins to 1.5 mm).
esp_board = [18.0, 22.6, 1.2, 8.94, 3.26, 0.6, 1.5, 1];  // ESP32-S3 SuperMini
tp_board  = [17.3, 28.0, 1.2, 8.94, 3.26, 0.6, 1.8, 0];  // TP4056 Type-C with protection
gy_board  = [16.4, 21.2, 1.6];                          // GY-521: width, length, thickness
gy_chip_h = 0.9;                                        // MPU6050 package height
bat_size  = [34, 51, 8];                                // LiPo: width, length, thickness
ss_body   = [3.6, 8.8, 3.9];   // slide switch: height without lever, length, width
ss_lever  = [4.0, 2.0, 1.6];   // lever: length, width (along travel), thickness
ss_travel = 2.0;
tact_body = 6.0;               // 6x6 switch body
tact_total_h = 8.0;            // bottom of the body -> top of the round actuator
tact_act_d = 3.5;
tact_pin = [6.5, 4.5];         // leg pitch: across the legs, along a row of legs

// ---------------- Clearances (per side) ----------------
fit = 0.2;          // printed part to printed part
pcb_clear = 0.3;    // board edge to its guides
usb_clear = 0.2;    // USB-C shell in its wall slot
ss_clear = 0.25;    // slide switch body in its pocket
tact_clear = 0.2;   // switch body in its socket
board_lift = 1.5;   // boards stand on ribs: solder joints underneath stay clear
pcb_edge_rest = 0.6; // pin-side-up boards rest on this strip along every PCB edge
                     // (keep parts and pads further in than this from the edge)

// ---------------- Case ----------------
case_len = 122;
case_wid = 44;
case_hgt = 21;
corner_r = 4;        // plan-view corner radius (more and the boards' corners hit the
                     // rounded inside corner: their USB end must touch the end wall)
edge_chamfer = 1.5;  // 45 degree chamfer on top/bottom edges: prints cleanly on the bed
wall = 2.0;
floor_t = 1.6;
top_t = 2.0;
split_z = 11;        // joint between bottom shell and top shell
lip_h = 2.0;         // alignment lip on the bottom shell, slides inside the top shell
lip_t = 1.0;

in_x0 = -case_wid / 2 + wall;  in_x1 = case_wid / 2 - wall;   // inner cavity
in_y0 = wall;                  in_y1 = case_len - wall;
in_z0 = floor_t;               in_z1 = case_hgt - top_t;

// ---------------- Screws: 4x M3 self-tapping, pan head, from the bottom ----------------
screw_len = 10;           // length of YOUR screws (thread part, without the head)
screw_bite = 5;           // thread length inside the top-shell boss
screw_xy = [[-15, 7.5], [15, 7.5], [-15, 91], [15, 91]];
screw_pilot_d = 2.5;      // bites into the top-shell boss
screw_pilot_depth = screw_bite + 1.5;   // room left past the screw tip
screw_clear_d = 3.4;
print_layer_h = 0.2;      // slicer layer height: the sacrificial bridge layers over screw pockets
screw_head_d = 6.2;
// Counterbore from the bottom face: deep enough that the screw reaches screw_bite
// into the top boss (10 mm screw -> 6 mm deep, 12 mm screw -> 4 mm deep).
screw_head_depth = split_z - (screw_len - screw_bite);
boss_top_d = 6.0;
boss_bot_d = 8.5;

// ---------------- Placement ----------------
tp_x = 0;            // charger: centred at the rear wall, USB-C = charging port
esp_x = -9;          // ESP32: front-left, USB-C at the tip = flashing port
gy_x = 10;           // GY-521: front-right, long side along the remote
bat_y0 = 31.6;       // battery bay between the stop ribs
bat_y1 = bat_y0 + bat_size[1] + 1.7;
ss_y = 20;           // power switch on the left wall
ss_z = in_z0 + 2.8 + ss_body[2] / 2;   // body sits on a 2.8 mm seat
usb_plug = [13.0, 7.5];  // opening for the plug's plastic overmould (w, h)

// ---------------- Buttons on the top face ----------------
// [name, x, y, cap diameter, symbol on the cap]
buttons = [
  ["AIR",   0,     109.5, 12, "AIR"],
  ["UP",    0,      95.5, 10, "up"],
  ["CLICK", 0,      83,   10, "OK"],
  ["LEFT",  -12.5,  83,   10, "left"],
  ["RIGHT", 12.5,   83,   10, "right"],
  ["DOWN",  0,      70.5, 10, "down"],
  ["PREV",  -10.5,  55,   12, "prev"],
  ["NEXT",  10.5,   55,   12, "next"],
  ["VOL-",  -12.5,  40,   10, "minus"],
  ["PLAY",  0,      40,   10, "playpause"],
  ["VOL+",  12.5,   40,   10, "plus"],
  ["FN",    0,      26,   10, "FN"],
];
// Switches drop in from the top face and rest on a ledge; legs go through it.
tact_recess = 1.5;   // body top below the case top -> actuator stands 3 mm proud
tact_ledge_t = 1.2;
tact_ledge_top = case_hgt - tact_recess - 3.5;   // body bottom (3.5 = common body height)
act_top_z = tact_ledge_top + tact_total_h;
sock_in = [tact_pin[0] + 0.7 + 2 * tact_clear, tact_body + 2 * tact_clear];  // legs must pass
sock_wall = 1.0;
cap_h = 3.5;
cap_socket_d = tact_act_d + 0.1;  // press fit on the actuator (+ a drop of glue)
cap_socket_depth = 2.0;
