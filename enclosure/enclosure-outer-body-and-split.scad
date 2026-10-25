// Shared shape helpers: rounded footprint, chamfered outer body, hollow shell,
// the split into bottom/top halves and the screw bosses.

// Case outline in plan view, grown (off > 0) or shrunk (off < 0) by `off`.
module footprint_2d(off = 0) {
  translate([-case_wid / 2 + corner_r, corner_r])
    offset(r = corner_r + off)
      square([case_wid - 2 * corner_r, case_len - 2 * corner_r]);
}

module rounded_rect_2d(size, r) {
  offset(r = r) square([size[0] - 2 * r, size[1] - 2 * r], center = true);
}

// Solid outer body with 45 degree chamfers on the top and bottom edges.
module outer_body() {
  c = edge_chamfer;
  hull() {
    linear_extrude(0.01) footprint_2d(-c);
    translate([0, 0, c]) linear_extrude(case_hgt - 2 * c) footprint_2d(0);
    translate([0, 0, case_hgt - 0.01]) linear_extrude(0.01) footprint_2d(-c);
  }
}

module inner_cavity() {
  translate([0, 0, in_z0]) linear_extrude(in_z1 - in_z0) footprint_2d(-wall);
}

module hollow_shell() {
  difference() {
    outer_body();
    inner_cavity();
  }
}

// Half-spaces used to cut the shell at split_z.
module below_z(z) { translate([-case_wid, -case_wid, -1]) cube([2 * case_wid, case_len + 2 * case_wid, z + 1]); }
module above_z(z) { translate([-case_wid, -case_wid, z]) cube([2 * case_wid, case_len + 2 * case_wid, case_hgt]); }

// Lip on the bottom shell: sits just inside the top shell's walls and lines both
// halves up. Its outer face leaves `fit` clearance to the top shell's inner wall.
module alignment_lip() {
  translate([0, 0, split_z - 0.01])
    linear_extrude(lip_h + 0.01)
      difference() {
        footprint_2d(-wall - fit);
        footprint_2d(-wall - fit - lip_t);
      }
}

// Shelf that carries the lip. The lip stands `fit` inside the wall's inner face, so on its
// own it floated over the cavity and the slicer grew tree supports all along both long
// walls. This shelf joins it to the wall with a 45 degree underside: no support needed.
// It reaches 0.1 mm past the lip (the lip always sits fully on it) and starts at
// split_z - 1.3 = 9.7 mm, above every part near the walls (highest: switch, 8.4 mm).
module lip_shelf() {
  w = fit + lip_t + 0.1;
  difference() {
    translate([0, 0, split_z - w]) linear_extrude(w) footprint_2d(-wall + 0.01);  // ends at split_z: the lip overlaps it
    hull() {  // footprint is convex: the hull of two slices is the 45 degree frustum
      translate([0, 0, split_z - w - 0.01]) linear_extrude(0.01) footprint_2d(-wall);
      translate([0, 0, split_z]) linear_extrude(0.02) footprint_2d(-wall - w);
    }
  }
}

// Bottom-shell bosses: solid from the floor to the joint.
module bottom_bosses() {
  for (p = screw_xy) translate([p[0], p[1], 0]) cylinder(d = boss_bot_d, h = split_z);
}

// Screw path in the bottom shell: head counterbore + clearance hole.
// Screws go in from below: head pocket, then two one-layer "sacrificial bridges" (a slot,
// then a square, both as wide as the screw) and the round hole above. A flat ring ceiling
// around the hole cannot be bridged, so the slicer filled every pocket with supports;
// these two layers print as plain bridges and the screw simply passes through them.
module bottom_screw_holes() {
  for (p = screw_xy) translate([p[0], p[1], 0]) {
    translate([0, 0, -1]) cylinder(d = screw_head_d, h = screw_head_depth + 1);
    translate([-screw_clear_d / 2, -screw_head_d / 2, screw_head_depth - 0.01])
      cube([screw_clear_d, screw_head_d, print_layer_h + 0.02]);     // layer 1: slot
    translate([-screw_clear_d / 2, -screw_clear_d / 2, screw_head_depth + print_layer_h - 0.01])
      cube([screw_clear_d, screw_clear_d, print_layer_h + 0.02]);    // layer 2: square
    translate([0, 0, screw_head_depth + 2 * print_layer_h])
      cylinder(d = screw_clear_d, h = split_z + lip_h + 2);
  }
}

// Top-shell bosses hang from the top plate down to the joint, with a pilot hole.
module top_bosses() {
  for (p = screw_xy)
    translate([p[0], p[1], split_z]) cylinder(d = boss_top_d, h = in_z1 - split_z + 0.01);
}

module top_screw_holes() {
  for (p = screw_xy)
    translate([p[0], p[1], split_z - 1]) cylinder(d = screw_pilot_d, h = screw_pilot_depth + 1);
}
