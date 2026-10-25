// Simplified models of the parts, only for previews, the 3D viewer and the fit check
// (they are never exported for printing). Sizes come from the MEASURE block.

// PCB + USB-C shell (sticking out past the USB edge) + a block standing in for the
// other parts on the component side (above the PCB, or below it when flipped).
// `only` draws a single piece ("pcb", "usb", "parts") for colour-coded sections.
mock_parts_h = 1.8;
module mock_usb_board(b, x_c, at_front, pcb_color, only = "all") {
  y0 = pcb_y0(b, at_front);
  usb_depth = 7.4;
  usb_y = at_front ? pcb_y1(b, true) - usb_depth + b[5] : y0 - b[5];
  parts_y = at_front ? y0 + 1 : y0 + usb_depth + 1;  // clear of the far-edge ledge
  parts_z = is_flipped(b) ? pcb_bottom(b) - mock_parts_h : pcb_top(b);
  if (only == "all" || only == "pcb")
    color(pcb_color) translate([x_c - b[0] / 2, y0, pcb_bottom(b)]) cube([b[0], b[1], b[2]]);
  if (only == "all" || only == "usb")
    color("silver") translate([x_c - b[3] / 2, usb_y, usb_bottom(b)]) cube([b[3], usb_depth, b[4]]);
  if (only == "all" || only == "parts")
    color("dimgray") translate([x_c - b[0] / 2 + 2, parts_y, parts_z])
      cube([b[0] - 4, b[1] - usb_depth - 2, mock_parts_h]);
}

module mock_charger() { mock_usb_board(tp_board, tp_x, false, "royalblue"); }
module mock_esp32() { mock_usb_board(esp_board, esp_x, true, "black"); }

module mock_battery() {
  color("silver", 0.9) translate([-bat_size[0] / 2, bat_y0 + 0.85, in_z0]) cube(bat_size);
}

module mock_gyro() {
  y0 = gy_y0 + pcb_clear;
  color("steelblue") translate([gy_x - gy_board[0] / 2, y0, pcb_z0()]) cube(gy_board);
  color("black") translate([gy_x - 2, y0 + gy_board[1] / 2 - 2, pcb_z0() + gy_board[2]])
    cube([4, 4, gy_chip_h]);
}

module mock_power_switch() {
  color("black") translate([in_x0, ss_y - ss_body[1] / 2, ss_z - ss_body[2] / 2]) cube(ss_body);
  color("white") translate([in_x0 - ss_lever[0], ss_y - ss_lever[1] / 2, ss_z - ss_lever[2] / 2])
    cube(ss_lever);
  color("silver") for (dy = [-2.5, 0, 2.5])  // 3 pins out of the back
    translate([in_x0 + ss_body[0], ss_y + dy - 0.25, ss_z - 0.2]) cube([3.5, 0.5, 0.4]);
}

// 6x6x8 switch resting on the socket ledge, legs through the ledge holes.
module mock_tact_switch() {
  z0 = tact_ledge_top;
  color("dimgray") translate([-tact_body / 2, -tact_body / 2, z0]) cube([tact_body, tact_body, 3.5]);
  color("black") translate([0, 0, z0]) cylinder(d = tact_act_d, h = tact_total_h);
  color("silver") for (sx = [-1, 1], sy = [-1, 1])
    translate([sx * tact_pin[0] / 2 - 0.15, sy * tact_pin[1] / 2 - 0.35, z0 - 3.5]) cube([0.3, 0.7, 3.5]);
}

module mock_components(with_caps = true) {
  mock_bottom_parts();
  mock_button_parts(with_caps);
}

// Charger, battery, ESP32, gyro and power switch (everything in the bottom shell).
module mock_bottom_parts() {
  mock_charger();
  mock_battery();
  mock_esp32();
  mock_gyro();
  mock_power_switch();
}

// Tactile switches in their sockets, optionally with the caps on.
module mock_button_parts(with_caps = true) {
  mock_tact_switches();
  if (with_caps) color("orange") assembled_caps();
}

module mock_tact_switches() {
  for (b = buttons) translate([b[1], b[2], 0]) mock_tact_switch();
}

// Caps sitting on the switch actuators, as in the finished remote.
module assembled_caps() {
  for (b = buttons)
    translate([b[1], b[2], act_top_z - cap_socket_depth]) button_cap(b[3], b[4]);
}
