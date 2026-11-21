// ===================================================================================
// Case for the C64 SD2IEC LP (github.com/wagiminator/C64-Collection/C64_SD2IEC_LP)
// Tray + lid + 3 button caps, all printable without supports on an FDM printer.
//   part = "tray"  -> print as exported (floor on the bed)
//   part = "lid"   -> exported upside down (outer face on the bed)
//   part = "caps"  -> three button caps, flange on the bed
//   part = "assembly" / "exploded" / "lidview" / "fitcheck" -> preview only
// Coordinates: X/Y = board seen from the component side, origin at the corner next to
// the ICSP header (cassette connector edge = Y 0, DIN/SD edge = Y max),
// Z = 0 at the solder side of the PCB.
// ===================================================================================
part = "assembly";

/* [Board] */
pcb_x   = 43.18;      // board width
pcb_y   = 55.88;      // board length (connector edge to DIN edge)
pcb_t   = 1.6;        // board thickness
gap     = 0.3;        // clearance around the board

/* [Component heights above the board (measured)] */
h_btn   = 5.0;        // tactile switches, top of the plunger
h_dip   = 6.0;        // DIP switch incl. sliders
h_jack  = 5.5;        // 3.5 mm jack
h_din   = 19.5;       // DIN socket (sticks out of the lid, only used for the preview)
h_back  = 2.0;        // tallest part / solder joint on the solder side

/* [Shell] */
wall    = 1.6;        // wall thickness (all walls, floor and lid)
floor_t = 1.6;
top_t   = 1.6;
air     = 0.3;        // clearance above the tallest enclosed part
cap_fl  = 0.5;        // flange thickness of the button caps above the plunger
back_h  = h_back + air;
front_h = max(h_jack, h_btn + cap_fl) + air;   // the DIP switch reaches into its opening in the lid
lap_h   = 2.4;        // height of the overlapping joint
fit     = 0.15;       // clearance between tray and lid
snap    = 0.25;       // height of the snap ridges
corner_r = 1.8;       // radius of the two outer corners at the DIN side
bed_ch  = 0.2;        // chamfer on all edges lying on the print bed (elephant foot compensation)

/* [Component positions] */
btn_c   = [[23.14, 31.04], [30.76, 31.04], [38.38, 31.04]];   // RESET, PREV, NEXT
btn_hole = 5.2;       // hole in the lid
cap_d   = 4.6;        // cap stem diameter
cap_out = 1.0;        // cap sticks out of the lid by this much
din_x   = [21.7, 43.1];       // DIN socket opening (front plate 21 mm wide)
din_y   = 36.3;               // rear end of the DIN socket opening
sd_x    = [3.35, 16.85];      // MicroSD slot in the lid wall
sd_h    = 2.4;                // slot height above the board
jack_y  = [15.8, 24.8];       // jack opening in the side wall
dip_c   = [38.0, 6.45];  dip_s = [7.8, 10.8];                 // DIP switch opening
icsp_c  = [5.08, 5.08];  icsp_s = [8.4, 5.7];                 // ICSP opening
led_c   = [10.0, 29.0];  led_s = [2.8, 5.6];                  // window above the three LEDs
pin_c   = [20.32, 53.34];     // free 2 mm hole in the board -> locating pin
pin_d   = 1.7;
stop_y  = 2.2;                // pull-out stop ends here (ICSP housing starts at Y = 2.54)

/* [Lettering] */
labels  = true;
label_d = 0.6;

$fn = 48;
e = 0.01;

x0 = -gap;             x1 = pcb_x + gap;           // cavity
y1 = pcb_y + gap;
ox0 = x0 - wall;       ox1 = x1 + wall;            // outer
oy1 = y1 + wall;
zf  = -back_h;                                     // inner floor
zs  = pcb_t;                                       // split plane = component side of the board
zc  = pcb_t + front_h;                             // inner ceiling
hw  = wall / 2;

module outline() {                                 // outer contour, connector end is open
  hull() {
    translate([ox0, 0]) square([ox1 - ox0, 1]);
    translate([ox0 + corner_r, oy1 - corner_r]) circle(corner_r);
    translate([ox1 - corner_r, oy1 - corner_r]) circle(corner_r);
  }
}
module cavity2d() { translate([x0, -1]) square([x1 - x0, y1 + 1]); }

// Chamfer cutter for the face lying on the print bed. children() = 2D contact area on the bed.
module bed_chamfer(z_bed, dir) {
  n = 4; st = bed_ch / n;
  if (bed_ch > 0) for (k = [0:n - 1])
    translate([0, 0, dir > 0 ? z_bed + k * st - (k == 0 ? e : 0) : z_bed - (k + 1) * st])
      linear_extrude(st + (k == 0 ? e : 0)) difference() {
        translate([ox0 - 5, -5]) square([ox1 - ox0 + 10, oy1 + 10]);
        offset(delta = -bed_ch * (1 - k / n)) children();
      }
}
module ridge(len, h) {                             // triangular snap ridge along Y (45 deg flanks)
  rotate([-90, 0, 0]) linear_extrude(len) polygon([[0, h + 0.2], [0, -h - 0.2], [h, 0]]);
}
snap_l = [[6, 9], [38, 9]];                        // left wall  [start, length]
snap_r = [[5, 8], [27.5, 7]];                      // right wall
snap_z = zs + lap_h / 2;

module din2d()  { translate([din_x[0], din_y]) square([din_x[1] - din_x[0], oy1 - din_y + 2]); }

// ----------------------------------------------------------------------------- symbols
sym_w = 1.5;
module stroke(a, b) { hull() { translate(a) circle(d = sym_w, $fn = 24); translate(b) circle(d = sym_w, $fn = 24); } }
module sym_tri()   { offset(r = 0.5, $fn = 24) offset(delta = -0.5) polygon([[-2.4, 3.0], [-2.4, -3.0], [2.7, 0]]); }
module sym_reset() {                               // open ring with a bar
  difference() { circle(d = 6.2); circle(d = 3.4); translate([-1.5, 0]) square([3.0, 4]); }
  stroke([0, 0.4], [0, 3.0]);
}
module sym_pins()  { for (x = [-2.5, 0, 2.5], y = [-1.25, 1.25]) translate([x, y]) circle(d = 1.7, $fn = 24); }
module labels2d() {
  sy = btn_c[0][1] - 7.3;
  translate([btn_c[0][0], sy]) sym_reset();                    // RESET
  translate([btn_c[1][0], sy]) mirror([1, 0]) sym_tri();       // PREV
  translate([btn_c[2][0], sy]) sym_tri();                      // NEXT
  translate([icsp_c[0] + 9.0, icsp_c[1]]) sym_pins();          // ICSP
  translate([16.5, 15.2]) text("SD2IEC", size = 4.6, font = "Liberation Sans:style=Bold", halign = "center", valign = "center", spacing = 1.08);
}
module openings2d() {                              // everything that goes through the lid plate
  din2d();
  for (c = btn_c) translate(c) circle(d = btn_hole);
  translate(dip_c) square(dip_s, center = true);
  translate(icsp_c) square(icsp_s, center = true);
  translate(led_c) offset(r = 0.8) offset(delta = -0.8) square(led_s, center = true);
}
module lid_bed2d() { difference() { outline(); openings2d(); if (labels) labels2d(); } }

// ----------------------------------------------------------------------------- tray
module tray() {
  difference() {
    union() {
      translate([0, 0, zf - floor_t]) linear_extrude(floor_t) outline();
      translate([0, 0, zf - e]) linear_extrude(zs - zf + e) difference() { outline(); cavity2d(); }
      // raised outer half of the side walls (joint)
      translate([0, 0, zs - e]) linear_extrude(lap_h + e) intersection() {
        outline();
        union() { translate([ox0, 0]) square([hw, oy1]); translate([ox1 - hw, 0]) square([hw, oy1]); }
      }
      // ledges carrying the board
      translate([0, 0, zf - e]) linear_extrude(back_h + e) {
        translate([x0, 0.5]) square([gap + 1.0, y1 - 0.5]);
        translate([x1 - gap - 1.0, 0.5]) square([gap + 1.0, y1 - 0.5]);
        translate([x0, pcb_y - 1.0]) square([x1 - x0, gap + 1.0]);
      }
      // post with locating pin for the free hole in the board
      translate([pin_c[0], pin_c[1], zf - e]) cylinder(d = 4.2, h = back_h + e);
      translate([pin_c[0], pin_c[1], -e]) cylinder(d = pin_d, h = pcb_t + 0.8);
    }
    // DIN socket: lower the wall below its front plate
    translate([din_x[0], pcb_y - 0.2, 0]) cube([din_x[1] - din_x[0] + 0.2, wall + gap + 2, 20]);
    // jack: cut the raised rim
    translate([x1 - 0.2, jack_y[0], zs]) cube([wall + 2, jack_y[1] - jack_y[0], 20]);
    // snap grooves
    for (s = snap_l) translate([ox0 + hw + e, s[0] - 0.3, snap_z]) mirror([1, 0, 0]) ridge(s[1] + 0.6, snap + 0.1);
    for (s = snap_r) translate([ox1 - hw - e, s[0] - 0.3, snap_z]) ridge(s[1] + 0.6, snap + 0.1);
    bed_chamfer(zf - floor_t, 1) outline();
  }
}

// ----------------------------------------------------------------------------- lid
module lid() {
  ix0 = ox0 + hw + fit;  ix1 = ox1 - hw - fit;     // outer faces of the inner wall halves
  difference() {
    union() {
      translate([0, 0, zc]) linear_extrude(top_t) outline();
      // full walls above the joint
      translate([0, 0, zs + lap_h + 0.1]) linear_extrude(zc - zs - lap_h - 0.1 + e)
        difference() { outline(); cavity2d(); }
      // inner halves of the side walls + DIN-side wall down to the board
      translate([0, 0, zs + 0.1]) linear_extrude(lap_h + e) difference() {
        intersection() { outline(); translate([ix0, 0]) square([ix1 - ix0, oy1]); }
        cavity2d();
      }
      // snap ridges
      for (s = snap_l) translate([ix0 + e, s[0], snap_z]) mirror([1, 0, 0]) ridge(s[1], snap + fit);
      for (s = snap_r) translate([ix1 - e, s[0], snap_z]) ridge(s[1], snap + fit);
      // end curtain above the cassette connector
      translate([x0 - e, 0, zs + 4.1]) cube([x1 - x0 + 2 * e, 1.0, zc - zs - 4.1 + e]);
      // pull-out stop in front of the ICSP header
      translate([x0 - e, 0.2, zs + 0.2]) cube([9.3 + gap, stop_y - 0.2, front_h]);
      // hold-down ribs on the board edges
      for (y = [10.5, 36]) translate([x0 - e, y, zs + 0.15]) cube([gap + 1.2, 2, front_h]);
      for (y = [12.8, 25.4]) translate([x1 - gap - 1.2 + e, y, zs + 0.15]) cube([gap + 1.2, 1.8, front_h]);
    }
    // openings in the plate
    translate([0, 0, zc - e]) linear_extrude(top_t + 1) openings2d();
    // DIN socket: remove the wall as well
    translate([0, 0, zs - 1]) linear_extrude(front_h + 2) din2d();
    // MicroSD slot (open towards the joint)
    translate([sd_x[0], pcb_y - 0.5, zs - 1]) cube([sd_x[1] - sd_x[0], wall + gap + 2, sd_h + 1]);
    // jack opening (open towards the joint)
    translate([x1 - 0.5, jack_y[0], zs - 1]) cube([wall + 2, jack_y[1] - jack_y[0], front_h + 1]);
    // symbols and lettering (debossed)
    if (labels) translate([0, 0, zc + top_t - label_d]) linear_extrude(label_d + e) labels2d();
    bed_chamfer(zc + top_t, -1) lid_bed2d();
  }
}

// ----------------------------------------------------------------------------- button caps
cap_fd  = 7.0;                                     // flange diameter
cap_rec = 0.6;                                     // recess for the plunger
module cap() {                                     // origin: bed, flange down
  stem = (front_h - h_btn - cap_fl) + top_t + cap_out;
  difference() {
    union() {
      cylinder(d = cap_fd, h = cap_rec + cap_fl);
      translate([0, 0, cap_rec + cap_fl - e]) cylinder(d = cap_d, h = stem - 0.4);
      translate([0, 0, cap_rec + cap_fl + stem - 0.4 - 2 * e]) cylinder(d1 = cap_d, d2 = cap_d - 0.8, h = 0.4);
    }
    translate([0, 0, -e]) cylinder(d = 3.9, h = cap_rec + e);
    for (k = [0:3]) translate([0, 0, k * bed_ch / 4 - e]) difference() {
      cylinder(d = cap_fd + 2, h = bed_ch / 4 + e); cylinder(d = cap_fd - 2 * bed_ch * (1 - k / 4), h = bed_ch / 4 + e); }
  }
}
module caps_placed() { for (c = btn_c) translate([c[0], c[1], zs + h_btn - cap_rec]) cap(); }

// ----------------------------------------------------------------------------- board dummy (fit check)
module board() {
  color("green") difference() { cube([pcb_x, pcb_y, pcb_t]); translate([pin_c[0], pin_c[1], -1]) cylinder(d = 2.0, h = 4); }
  color("dimgray") { translate([23.5, 36.75, pcb_t]) cube([17.8, pcb_y - 36.75, h_din]);                       // DIN socket
                     translate([21.9, pcb_y - 0.2, 0.2]) cube([21.0, 1.9, pcb_t + h_din - 0.2]); }
  color("silver") translate([2.95, 42.3, pcb_t]) cube([14.3, 13.6, 1.9]);                                      // MicroSD socket
  color("dimgray") translate([4.6, 46, pcb_t + 0.4]) cube([11.0, 15, 1.0]);                                    // MicroSD card
  color("silver") for (c = btn_c) translate([c[0] - 3.1, c[1] - 3.1, pcb_t]) cube([6.2, 6.2, 3.6]);            // switches
  color("black")  for (c = btn_c) translate([c[0], c[1], pcb_t]) cylinder(d = 3.5, h = h_btn);
  color("black")  { translate([29.2, 17.4, pcb_t]) cube([11.7, 5.8, h_jack]);                                  // jack
                    translate([40.9, 20.3, pcb_t + 2.5]) rotate([0, 90, 0]) cylinder(d = 5.0, h = 3.5); }
  color("red")    translate([34.7, 1.4, pcb_t]) cube([6.6, 9.9, h_dip]);                                       // DIP switch
  color("black")  translate([1.27, 2.54, pcb_t]) cube([7.62, 5.08, 2.54]);                                     // ICSP housing
  color("gold")   for (x = [2.54, 5.08, 7.62], y = [3.81, 6.35]) translate([x - 0.32, y - 0.32, pcb_t]) cube([0.64, 0.64, 8.6]);
  color("silver") translate([4.0, 14.7, pcb_t]) cube([4.7, 11.5, 3.6]);                                        // crystal
  color("white")  for (y = [27.2, 29.0, 30.8]) translate([9.2, y - 0.4, pcb_t]) cube([1.6, 0.8, 0.6]);         // LEDs
  color("silver") translate([1.3, 1.3, -h_back]) cube([40.5, 53.4, h_back - 0.1]);                             // solder side
  color("blue")   translate([pcb_x / 2 - 23.25, -8, pcb_t / 2 - 4.6]) cube([46.5, 8, 9.2]);                    // edge connector
  color("blue")   translate([pcb_x / 2 - 16.5, -12.5, pcb_t / 2 - 4.6]) cube([33, 4.5, 9.2]);
}
module board_fit() { difference() { board(); translate([pin_c[0], pin_c[1], -10]) cylinder(d = 4.6, h = 10); } }

if (part == "tray") translate([0, 0, back_h + floor_t]) tray();
else if (part == "lid") translate([0, 0, zc + top_t]) rotate([0, 180, 0]) translate([-pcb_x, 0, 0]) lid();
else if (part == "caps") for (i = [0:2]) translate([i * (cap_fd + 3), 0, 0]) cap();
else if (part == "lidview") lid();
else if (part == "fitcheck") intersection() { union() { tray(); lid(); caps_placed(); } board_fit(); }
else if (part == "exploded") { color("gainsboro") tray(); board(); color("orange") translate([0, 0, 10]) caps_placed(); color("gainsboro") translate([0, 0, 16]) lid(); }
else { color("gainsboro") tray(); board(); color("orange") caps_placed(); color("gainsboro", 0.85) lid(); }
