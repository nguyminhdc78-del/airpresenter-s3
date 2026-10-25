// Top shell: top plate, 12 switch sockets, screw bosses, the tongues that close the
// USB and lever slots of the bottom shell, and the posts that keep parts down.
//
// Socket: each 6x6x8 switch is pushed in FROM THE TOP FACE until it rests on a ledge;
// its 4 legs pass through holes in the ledge. Pressing a button pushes the switch
// onto the ledge, so it can never be pushed into the case. Solder the wires to the
// legs under the ledge (or bend the legs) and the switch cannot come back out.

module tact_socket() {
  hx = sock_in[0] / 2;
  hy = sock_in[1] / 2;
  z0 = tact_ledge_top - tact_ledge_t;
  difference() {
    translate([-hx - sock_wall, -hy - sock_wall, z0])
      cube([sock_in[0] + 2 * sock_wall, sock_in[1] + 2 * sock_wall, in_z1 - z0 + 0.01]);
    translate([-hx, -hy, tact_ledge_top]) cube([sock_in[0], sock_in[1], case_hgt]);
    // Leg holes stop one layer short of the seat. Printed upside down, the seat is the
    // pocket's ceiling: a solid first layer bridges in one go (with the holes through it the
    // slicer put supports in all 12 pockets). The switch legs pierce that 0.2 mm skin
    // (or prick it first with a pin / hot soldering iron tip).
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * tact_pin[0] / 2 - 0.7, sy * tact_pin[1] / 2 - 0.6, z0 - 1])
        cube([1.4, 1.2, tact_ledge_t + 1 - print_layer_h]);
  }
}

module top_shell() {
  difference() {
    union() {
      intersection() {
        hollow_shell();
        above_z(split_z);
      }
      for (b = buttons) translate([b[1], b[2], 0]) tact_socket();
      top_bosses();
      usb_port_tongue(tp_board, tp_x, false);
      usb_port_tongue(esp_board, esp_x, true);
      lever_port_tongue();
      holder_lid_posts();
    }
    for (b = buttons)  // the switch goes in through the top face
      translate([b[1] - sock_in[0] / 2, b[2] - sock_in[1] / 2, tact_ledge_top]) cube([sock_in[0], sock_in[1], case_hgt]);
    top_screw_holes();
  }
}

// Top shell turned over so the flat top face lies on the print bed.
module top_shell_print() {
  translate([0, case_len, case_hgt]) rotate([180, 0, 0]) top_shell();
}
