// AirPresenter S3 enclosure - 3D-printable case for the BLE presenter remote.
// Open in OpenSCAD, pick `part`, press F6 (render) and export STL.
//   fit_test                        : print this FIRST and try your parts in it
//   bottom_print / top_print / caps : printable parts, already in print orientation
//   assembly / exploded / layout    : previews with mock components
//   fit_check_*                     : must render with no volume (nothing overlaps)

include <enclosure-parameters.scad>
include <enclosure-outer-body-and-split.scad>
include <enclosure-board-cradles-and-usb-ports.scad>
include <enclosure-switch-gyro-battery-holders.scad>
include <enclosure-bottom-shell.scad>
include <enclosure-top-shell.scad>
include <enclosure-button-caps.scad>
include <enclosure-component-mockups.scad>
include <enclosure-fit-test-coupon.scad>

// Close-up cut through one button: the switch resting on its ledge, legs through it.
module socket_section(b) {
  module cut() { translate([b[1] - 14, b[2] - 5.5, 10]) cube([14 + 3.25, 11, 20]); }  // through the legs
  color("gainsboro") intersection() { top_shell(); cut(); }
  color("dimgray") intersection() { translate([b[1], b[2], 0]) mock_tact_switch(); cut(); }
  color("orange") intersection() {
    translate([b[1], b[2], act_top_z - cap_socket_depth]) button_cap(b[3], b[4]);
    cut();
  }
}

// Cut along the ESP32's centre line: pin side up, USB-C shell under the PCB on its pad,
// clamped in the wall slot by the top shell's tongue.
module esp32_section() {
  module cut() { translate([esp_x - 20, case_len - 30, -1]) cube([20, 31, case_hgt + 2]); }
  color("lightgray") intersection() { bottom_shell(); cut(); }
  color("white") intersection() { top_shell(); cut(); }
  color("royalblue") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "pcb"); cut(); }
  color("silver") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "usb"); cut(); }
  color("dimgray") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "parts"); cut(); }
}

// Cut across the width through the middle of the ESP32: PCB resting on both long-edge
// rails, the parts under it hanging in between, the AIR button above.
module width_section() {
  y_cut = (pcb_y0(esp_board, true) + pcb_y1(esp_board, true)) / 2;
  module cut() { translate([-case_wid, y_cut, -1]) cube([2 * case_wid, 30, case_hgt + 10]); }
  color("lightgray") intersection() { bottom_shell(); cut(); }
  color("white") intersection() { top_shell(); cut(); }
  color("royalblue") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "pcb"); cut(); }
  color("silver") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "usb"); cut(); }
  color("dimgray") intersection() { mock_usb_board(esp_board, esp_x, true, "black", "parts"); cut(); }
  color("steelblue") intersection() { mock_gyro(); cut(); }
  color("dimgray") intersection() { mock_tact_switches(); cut(); }
  color("orange") intersection() { assembled_caps(); cut(); }
}

// The ESP32 corner of the bottom shell with the board lifted out: USB pad, far-edge
// ledge and the two stepped long-edge rails. with_pcb = show only the PCB outline.
module esp32_cradle_view(with_pcb = false) {
  color("lightgray") intersection() {
    bottom_shell();
    translate([-case_wid / 2 - 1, pcb_y0(esp_board, true) - 6, -1]) cube([24, 40, case_hgt]);
  }
  if (with_pcb) color("royalblue") mock_usb_board(esp_board, esp_x, true, "royalblue", "pcb");
}

part = "assembly";  // [assembly, exploded, layout, socket_section, esp32_section, width_section, esp32_cradle, fit_test, bottom_print, top_print, caps, fit_check_bottom, fit_check_top, fit_check_shells]

if (part == "bottom_print") bottom_shell();
else if (part == "top_print") top_shell_print();
else if (part == "caps") button_caps_plate();
else if (part == "fit_test") fit_test_coupon();
else if (part == "assembly") {
  color("lightgray") bottom_shell();
  color("gainsboro") top_shell();
  mock_components();
}
else if (part == "exploded") {
  color("lightgray") bottom_shell();
  mock_bottom_parts();
  translate([0, 0, 30]) {
    color("gainsboro", 0.5) top_shell();
    mock_button_parts(with_caps = false);
    color("orange") translate([0, 0, 8]) assembled_caps();
  }
}
else if (part == "layout") {  // top view: where each function sits
  color("gainsboro") top_shell();
  color("orange") assembled_caps();
}
else if (part == "socket_section") socket_section(buttons[9]);
else if (part == "esp32_section") esp32_section();
else if (part == "width_section") width_section();
else if (part == "esp32_cradle") esp32_cradle_view();
else if (part == "fit_check_bottom") intersection() { bottom_shell(); mock_components(); }
else if (part == "fit_check_top") intersection() { top_shell(); mock_components(); }
else if (part == "fit_check_shells") intersection() { top_shell(); bottom_shell(); }
