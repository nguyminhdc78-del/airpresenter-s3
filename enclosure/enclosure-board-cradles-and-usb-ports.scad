// Glue-free holders for the two USB-C boards (ESP32-S3 SuperMini, TP4056).
//
// How a board is held:
//  - its USB-C shell drops into a U-slot cut down from the top of the end wall;
//    the top shell has a tongue that fills the slot and presses on the shell, so the
//    USB end is clamped up/down and left/right;
//  - side guides stop sideways play, a stop rib takes the push of the plug;
//  - a post on the top shell sits just above the far end (it never presses parts).
// Component side up (flipped = 0): the PCB stands 1.5 mm up on a ledge + a rib, so
// solder joints underneath stay clear.
// Pin side up (flipped = 1): the USB-C shell hangs under the PCB and rests on a pad;
// the PCB rests on all four edges: a ledge under the far edge and a stepped rail
// under each long edge (low step under the outer pcb_edge_rest strip, high step =
// side wall outside the edge). The parts hanging under the PCB sit in between.
// Board vector b = [pcb w, pcb l, pcb t, usb w, usb h, usb overhang,
//                   tallest thing on the face that points up, flipped].

function is_flipped(b) = b[7] == 1;
function pcb_z0() = in_z0 + board_lift;  // component side up (also used by the GY-521)
// Flipped: lift the USB-C shell so the plug opening stays clear of the floor.
function usb_bottom(b) = is_flipped(b)
  ? in_z0 + max(0.8, usb_plug[1] / 2 - b[4] / 2 + 0.2)
  : pcb_z0() + b[2];
function usb_top(b) = usb_bottom(b) + b[4];
function usb_zc(b) = usb_bottom(b) + b[4] / 2;
function pcb_bottom(b) = is_flipped(b) ? usb_top(b) : pcb_z0();
function pcb_top(b) = pcb_bottom(b) + b[2];
// The USB end of the PCB touches the inner face of its end wall.
function pcb_y0(b, at_front) = at_front ? in_y1 - b[1] : in_y0;
function pcb_y1(b, at_front) = at_front ? in_y1 : in_y0 + b[1];
function usb_slot_w(b) = b[3] + 2 * usb_clear;

// Low rib standing on the floor, from (x0, y0) to (x1, y1), top at z_top.
module floor_rib(x0, x1, y0, y1, z_top) {
  translate([min(x0, x1), min(y0, y1), in_z0 - 0.01])
    cube([abs(x1 - x0), abs(y1 - y0), z_top - in_z0 + 0.01]);
}

module board_cradle(b, x_c, at_front) {
  y0 = pcb_y0(b, at_front);
  y1 = pcb_y1(b, at_front);
  usb_y = at_front ? y1 : y0;
  far_y = at_front ? y0 : y1;
  inward = at_front ? -1 : 1;  // from the end wall into the case
  top = pcb_top(b);
  if (is_flipped(b)) {
    // pad under the hanging USB-C shell, ledge under the far edge
    floor_rib(x_c - b[3] / 2 + 0.5, x_c + b[3] / 2 - 0.5, usb_y, usb_y + inward * 6, usb_bottom(b));
    floor_rib(x_c - b[0] / 2, x_c + b[0] / 2, far_y, far_y + inward * pcb_edge_rest, pcb_bottom(b));
    for (side = [-1, 1]) {  // low step of the long-edge rails, full length
      edge = x_c + side * b[0] / 2;
      floor_rib(edge - side * pcb_edge_rest, edge + side * (pcb_clear + 1.2), y0, y1, pcb_bottom(b));
    }
  } else {
    // ledge under the USB end, narrow enough to miss the USB shell's solder legs
    floor_rib(x_c - 3, x_c + 3, usb_y, usb_y + inward * 1.2, pcb_bottom(b));
    floor_rib(x_c - 4, x_c + 4, far_y, far_y - inward * 1.5, pcb_bottom(b));
  }
  // stop rib right behind the far end
  floor_rib(x_c - 4, x_c + 4, far_y + inward * pcb_clear, far_y + inward * (pcb_clear + 1.2), top + 1.5);
  for (side = [-1, 1]) {  // side guides
    gx = x_c + side * (b[0] / 2 + pcb_clear);
    floor_rib(gx, gx + side * 1.2, min(y0, y1) + 3, max(y0, y1) - 3, top + 0.8);
  }
}

// Opening for the plug's plastic overmould: from outside up to the USB-C face.
module usb_plug_bore(b, x_c, at_front, roof = true) {
  depth = max(wall - b[5], 0.4);
  outer = at_front ? case_len : 0;
  inward = at_front ? -1 : 1;
  y_a = outer - inward * 1;
  y_b = outer + inward * depth;
  // The opening is wider than the U-slot above it: in the bottom shell a 45 degree roof from
  // its top corners up to the slot width lets it print without support (a flat roof there
  // needed some). The top shell's tongue keeps the plain rounded opening (roof = false).
  rise = roof ? (usb_plug[0] - usb_slot_w(b)) / 2 : 0;
  translate([x_c, (y_a + y_b) / 2, usb_zc(b)]) rotate([90, 0, 0])
    linear_extrude(abs(y_b - y_a), center = true) hull() {
      rounded_rect_2d(usb_plug, 2);
      if (rise > 0) translate([0, usb_plug[1] / 2 + rise / 2]) square([usb_slot_w(b), rise], center = true);
    }
}

// Cut in the bottom shell: overmould opening + U-slot for the USB-C shell.
module usb_port_cutout(b, x_c, at_front) {
  usb_plug_bore(b, x_c, at_front);
  w = usb_slot_w(b);
  y_wall = at_front ? in_y1 - 0.01 : -1;
  translate([x_c - w / 2, y_wall, usb_bottom(b) - 0.15])
    cube([w, wall + 1.01, split_z + lip_h + 1 - usb_bottom(b)]);
}

// Tongue on the top shell: fills the U-slot down to just above the USB-C shell.
module usb_port_tongue(b, x_c, at_front) {
  w = usb_slot_w(b) - 2 * fit;
  z0 = usb_top(b) + 0.1;
  y_wall = at_front ? in_y1 : 0;
  difference() {
    translate([x_c - w / 2, y_wall, z0]) cube([w, wall, split_z - z0 + 0.01]);
    usb_plug_bore(b, x_c, at_front, roof = false);
  }
}

// Post hanging from the top plate, ending just above whatever points up on the board.
module board_lid_post(b, x_c, y_c) {
  z0 = pcb_top(b) + b[6] + 0.3;
  translate([x_c - 1.5, y_c - 1.5, z0]) cube([3, 3, in_z1 - z0 + 0.01]);
}
