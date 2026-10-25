// Fit test coupon (~1 hour of printing): real slices of both shells, cut from the
// same geometry as the full case, so a part that fits here fits in the case.
//   A  rear of the bottom shell: charger cradle + charging-port slot + switch pocket
//   B  front of the bottom shell: ESP32 + GY-521 cradles + flashing-port slot
//   C  a row of 3 button sockets (VOL-, PLAY, VOL+), top face down
//   D  rear of the top shell: charging-port tongue, lever tongue, posts; fits onto A
//   E  one cap of each size

// Band of the top shell between y0 and y1, turned top face down, starting at y = 0.
module coupon_top_slice(y0, y1) {
  translate([0, y1 - y0, case_hgt]) rotate([180, 0, 0])
    translate([0, -y0, 0]) intersection() {
      top_shell();
      translate([-case_wid, y0, -1]) cube([2 * case_wid, y1 - y0, case_hgt + 2]);
    }
}

module coupon_bottom_slice(y0, y1) {
  translate([0, -y0, 0]) intersection() {
    bottom_shell();
    translate([-case_wid, y0, -1]) cube([2 * case_wid, y1 - y0, case_hgt + 2]);
  }
}

module fit_test_coupon() {
  coupon_bottom_slice(-1, 34);                                // A
  translate([0, 40, 0]) coupon_bottom_slice(case_len - 30, case_len + 1);  // B
  translate([0, 76, 0]) coupon_top_slice(34, 46);             // C
  translate([0, 94, 0]) coupon_top_slice(-1, 25);             // D
  translate([-8, 128, 0]) button_cap(10, "OK");               // E
  translate([8, 128, 0]) button_cap(12, "AIR");
}
