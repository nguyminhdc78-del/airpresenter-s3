// Button caps: press onto the 3.6 mm round actuator of each switch (add a drop of
// CA glue). The symbol is engraved 0.6 mm into the top. Printed top-side up.

module cap_symbol_2d(sym, d) {
  s = d * 0.42;
  if (sym == "up") polygon([[-s / 2, -s / 3], [s / 2, -s / 3], [0, s / 2]]);
  else if (sym == "down") rotate(180) cap_symbol_2d("up", d);
  else if (sym == "left") rotate(90) cap_symbol_2d("up", d);
  else if (sym == "right") rotate(-90) cap_symbol_2d("up", d);
  else if (sym == "next") {
    for (dx = [-s * 0.27, s * 0.27]) translate([dx, 0]) cap_symbol_2d("right", d * 0.75);
  }
  else if (sym == "prev") mirror([1, 0]) cap_symbol_2d("next", d);
  else if (sym == "minus") square([s * 1.1, s * 0.24], center = true);
  else if (sym == "plus") {
    square([s * 1.1, s * 0.24], center = true);
    square([s * 0.24, s * 1.1], center = true);
  }
  else if (sym == "playpause") {
    translate([-s * 0.3, 0]) cap_symbol_2d("right", d * 0.8);
    for (dx = [s * 0.18, s * 0.42]) translate([dx, 0]) square([s * 0.13, s * 0.62], center = true);
  }
  else text(sym, size = len(sym) > 2 ? d * 0.27 : d * 0.38, font = "Liberation Sans:style=Bold",
            halign = "center", valign = "center");
}

module button_cap(d, sym) {
  chamfer = 0.6;
  difference() {
    union() {
      cylinder(d = d, h = cap_h - chamfer);
      translate([0, 0, cap_h - chamfer]) cylinder(d1 = d, d2 = d - 2 * chamfer, h = chamfer);
    }
    translate([0, 0, -0.01]) cylinder(d = cap_socket_d, h = cap_socket_depth + 0.01);
    translate([0, 0, cap_h - 0.6]) linear_extrude(1) cap_symbol_2d(sym, d);
  }
}

// All 12 caps on one plate, 4 per row.
module button_caps_plate() {
  for (i = [0 : len(buttons) - 1]) {
    b = buttons[i];
    translate([(i % 4) * 15, floor(i / 4) * 15, 0]) button_cap(b[3], b[4]);
  }
}
