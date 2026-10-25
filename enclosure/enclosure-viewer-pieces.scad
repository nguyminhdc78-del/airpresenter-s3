// One piece of the remote, in its assembled position, for the web 3D viewer
// (enclosure/viewer). Exported by viewer/export-viewer-models.sh, one STL per piece.

include <enclosure-parameters.scad>
include <enclosure-outer-body-and-split.scad>
include <enclosure-board-cradles-and-usb-ports.scad>
include <enclosure-switch-gyro-battery-holders.scad>
include <enclosure-bottom-shell.scad>
include <enclosure-top-shell.scad>
include <enclosure-button-caps.scad>
include <enclosure-component-mockups.scad>

piece = "bottom_shell";  // [bottom_shell, top_shell, caps, tact_switches, battery, charger, esp32, gyro, power_switch]

if (piece == "bottom_shell") bottom_shell();
else if (piece == "top_shell") top_shell();
else if (piece == "caps") assembled_caps();
else if (piece == "tact_switches") mock_tact_switches();
else if (piece == "battery") mock_battery();
else if (piece == "charger") mock_charger();
else if (piece == "esp32") mock_esp32();
else if (piece == "gyro") mock_gyro();
else if (piece == "power_switch") mock_power_switch();
