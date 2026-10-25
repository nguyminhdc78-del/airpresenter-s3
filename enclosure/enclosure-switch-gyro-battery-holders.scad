// Glue-free holders for the power switch, the GY-521 and the battery, plus the
// posts on the top shell that stop them lifting once the case is closed.

// ---------- SS-12D01 power switch on the left wall ----------
// The switch drops in from above: lever into a U-slot in the wall, body into a pocket
// (seat below, end stops, back ribs that leave room for the 3 pins). The top shell's
// tongue closes the U-slot above the lever's travel channel.
ss_len = ss_body[1] + 2 * ss_clear;
ss_back_x = in_x0 + ss_body[0] + ss_clear;      // back face of the pocket
ss_top = ss_z + ss_body[2] / 2;
lever_slot_len = ss_lever[1] + ss_travel + 0.6;
lever_slot_z0 = ss_z - ss_lever[2] / 2 - 0.3;
lever_slot_z1 = ss_z + ss_lever[2] / 2 + 0.3;

module power_switch_pocket() {
  y0 = ss_y - ss_len / 2;
  y1 = ss_y + ss_len / 2;
  floor_rib(in_x0 - 0.01, ss_back_x + 1.2, y0, y1, ss_z - ss_body[2] / 2);  // seat
  for (s = [-1, 1]) {
    floor_rib(in_x0 - 0.01, ss_back_x + 1.2, s > 0 ? y1 : y0, s > 0 ? y1 + 1.2 : y0 - 1.2, ss_top);
    floor_rib(ss_back_x, ss_back_x + 1.2, ss_y + s * 3.3, s > 0 ? y1 : y0, ss_top);  // back ribs
  }
}

module lever_port_cutout() {
  translate([-case_wid / 2 - 1, ss_y - lever_slot_len / 2, lever_slot_z0])
    cube([wall + 1.01, lever_slot_len, split_z + lip_h + 1 - lever_slot_z0]);
}

module lever_port_tongue() {
  translate([-case_wid / 2, ss_y - lever_slot_len / 2 + fit, lever_slot_z1])
    cube([wall, lever_slot_len - 2 * fit, split_z - lever_slot_z1 + 0.01]);
}

// ---------- GY-521, front right, long side along the remote ----------
// Board stands on two pads, located by four L-shaped corner posts.
gy_in = [gy_board[0] + 2 * pcb_clear, gy_board[1] + 2 * pcb_clear];
gy_y1 = in_y1 - 0.3;
gy_y0 = gy_y1 - gy_in[1];

module gyro_cradle() {
  x0 = gy_x - gy_in[0] / 2;
  x1 = gy_x + gy_in[0] / 2;
  top = pcb_z0() + gy_board[2] + 1.0;
  for (y = [gy_y0 + pcb_clear, gy_y1 - pcb_clear - 1.5])  // pads under the short edges
    floor_rib(gy_x - 3, gy_x + 3, y, y + 1.5, pcb_z0());
  for (cx = [x0, x1], cy = [gy_y0, gy_y1]) {
    sx = cx == x0 ? -1 : 1;  // outward directions of this corner
    sy = cy == gy_y0 ? -1 : 1;
    floor_rib(cx, cx + sx * 1.0, cy + sy * 1.0, cy - sy * 2.5, top);
    floor_rib(cx + sx * 1.0, cx - sx * 2.5, cy, cy + sy * 1.0, top);
  }
}

// ---------- Battery bay ----------
bat_hw = bat_size[0] / 2 + 0.4;

module battery_bay() {
  rib_top = in_z0 + 4.4;
  for (s = [-1, 1]) {
    floor_rib(s * bat_hw, s * (bat_hw + 1.2), bat_y0 + 6, bat_y1 - 6, rib_top);  // sides
    floor_rib(s * 5, s * bat_hw, bat_y0 - 1.2, bat_y0, rib_top);  // rear stop, wire gap
  }
  floor_rib(-10, 10, bat_y1, bat_y1 + 1.2, rib_top);  // front stop, clear of the bosses
}

// ---------- Posts on the top shell ----------
module holder_lid_posts() {
  board_lid_post(tp_board, tp_x + 7, 18);
  board_lid_post(esp_board, esp_x, pcb_y0(esp_board, true) + 3.5);
  gy_post_z = pcb_z0() + gy_board[2] + gy_chip_h + 0.4;
  translate([gy_x - 1.5, (gy_y0 + gy_y1) / 2 - 1.5, gy_post_z]) cube([3, 3, in_z1 - gy_post_z + 0.01]);
  // above the switch body, inside the bottom shell's alignment lip
  translate([in_x0 + lip_t + fit + 0.2, ss_y - 1.5, ss_top + 0.5]) cube([3, 3, in_z1 - ss_top - 0.5 + 0.01]);
  for (y = [47.5, 62.75]) {  // battery keepers, between the button sockets
    z0 = in_z0 + bat_size[2] + 1.2;
    translate([-1.5, y - 1.5, z0]) cube([3, 3, in_z1 - z0 + 0.01]);
  }
}
