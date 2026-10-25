// Bottom shell: floor, lower walls, alignment lip, screw bosses and the glue-free
// holders (see enclosure-board-cradles-and-usb-ports.scad and
// enclosure-switch-gyro-battery-holders.scad for how each part is held).

module bottom_shell() {
  difference() {
    union() {
      intersection() {
        hollow_shell();
        below_z(split_z);
      }
      alignment_lip();
      lip_shelf();
      intersection() {  // keep bosses inside the outer body
        bottom_bosses();
        outer_body();
      }
      board_cradle(tp_board, tp_x, false);
      board_cradle(esp_board, esp_x, true);
      gyro_cradle();
      power_switch_pocket();
      battery_bay();
    }
    bottom_screw_holes();
    usb_port_cutout(tp_board, tp_x, false);    // charging port, rear end
    usb_port_cutout(esp_board, esp_x, true);   // flashing port, front tip
    lever_port_cutout();                       // power switch lever, left wall
  }
}
