// ===================================================================================
// Case for the C64 Tapuino (github.com/wagiminator/C64-Collection/C64_Tapuino)
// Two parts (tray + lid), both printable without supports on an FDM printer.
//   part = "tray"  -> print as exported (floor on the bed)
//   part = "lid"   -> exported upside down (outer face on the bed)
//   part = "assembly" / "fitcheck" -> preview only
// Coordinates: X/Y = board seen from the display side, origin at the corner next to
// the ICSP header (cassette connector edge = Y 0), Z = 0 at the rear side of the PCB.
// ===================================================================================
part = "assembly";

/* [Board] */
pcb_x   = 43.18;      // board width
pcb_y   = 45.72;      // board length (connector edge to button edge)
pcb_t   = 1.6;        // board thickness
gap     = 0.3;        // clearance around the board

/* [Shell] */
wall    = 2.4;        // side wall thickness (split into two 1.2 mm halves at the joint)
wall_b  = 1.6;        // wall thickness at the button side (thin, so the plungers stay reachable)
floor_t = 1.6;        // tray floor thickness
top_t   = 1.6;        // lid thickness
back_h  = 4.0;        // free height below the board: optocoupler 3.7 mm + 0.3 mm
front_h = 5.0;        // free height above the board: buttons 4.7 mm + 0.3 mm (OLED 4.3 mm)
lap_h   = 2.4;        // height of the overlapping joint
fit     = 0.15;       // clearance between tray and lid
snap    = 0.3;        // height of the snap ridges
corner_r = 2.5;       // radius of the two outer corners at the button side
bed_ch  = 0.2;        // chamfer on all edges lying on the print bed (elephant foot compensation)

/* [Buttons: 4x right-angle tactile switch] */
btn_x   = [6.35, 16.51, 26.67, 36.83];
btn_w   = 8.2;        // width needed per button
btn_teeth = false;    // true: separate cut-out per button, false: one common opening

/* [OLED window] */
win_c   = [21.59, 21.97];   // centre of the visible display area
win_s   = [24.5, 13.0];     // window size
win_ch  = 1.0;              // bezel chamfer

/* [ICSP header opening and pull-out stop] */
icsp_c  = [3.81, 6.35];     // centre of the 2x3 header
icsp_s  = [5.8, 8.1];       // opening in the lid
stop_y  = 2.2;              // stop block ends here (header housing starts at Y = 2.54)

/* [MicroSD slot in the tray wall] */
sd_y    = [18.85, 32.35];   // slot range along the wall
sd_z    = 2.4;              // slot depth below the board

/* [Lettering] */
labels  = true;
label_d = 0.6;

$fn = 48;
e = 0.01;

x0 = -gap;             x1 = pcb_x + gap;           // cavity
y1 = pcb_y + gap;
ox0 = x0 - wall;       ox1 = x1 + wall;            // outer
oy1 = y1 + wall_b;
zf  = -back_h;                                     // inner floor
zs  = pcb_t;                                       // split plane = top side of the board
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
// dir = +1: part grows upwards from z_bed, dir = -1: part grows downwards (lid, printed upside down).
module bed_chamfer(z_bed, dir) {
  n = 4; st = bed_ch / n;
  if (bed_ch > 0) for (k = [0:n - 1])
    translate([0, 0, dir > 0 ? z_bed + k * st - (k == 0 ? e : 0) : z_bed - (k + 1) * st])
      linear_extrude(st + (k == 0 ? e : 0)) difference() {
        translate([ox0 - 5, -5]) square([ox1 - ox0 + 10, oy1 + 10]);
        offset(delta = -bed_ch * (1 - k / n)) children();
      }
}

// button opening: the lid ends at the board edge there, so the plungers are easy to reach
module btn_recess2d() {
  if (btn_teeth) for (x = btn_x) translate([x - btn_w / 2, pcb_y]) square([btn_w, wall_b + gap + 3]);
  else translate([btn_x[0] - btn_w / 2, pcb_y]) square([btn_x[3] - btn_x[0] + btn_w, wall_b + gap + 3]);
}
// Symbols instead of small lettering (bold enough for a 0.4 mm nozzle):
// NEXT = triangle down, PREV = triangle up, BACK = cross, SELECT = check mark, ICSP = 2x3 pin pattern
sym_y  = 40.3;        // centre line of the button symbols
sym_w  = 1.5;         // stroke width of cross and check mark
module stroke(a, b) { hull() { translate(a) circle(d = sym_w, $fn = 24); translate(b) circle(d = sym_w, $fn = 24); } }
module sym_tri()   { offset(r = 0.5, $fn = 24) offset(delta = -0.5) polygon([[-3.1, 2.5], [3.1, 2.5], [0, -2.7]]); }
module sym_cross() { stroke([-2.2, -2.2], [2.2, 2.2]); stroke([-2.2, 2.2], [2.2, -2.2]); }
module sym_check() { stroke([-2.7, -0.2], [-1.0, -2.1]); stroke([-1.0, -2.1], [2.7, 2.2]); }
module sym_pins()  { for (x = [-1.25, 1.25], y = [-2.5, 0, 2.5]) translate([x, y]) circle(d = 1.7, $fn = 24); }
module labels2d() {
  translate([btn_x[0], sym_y]) sym_tri();                      // NEXT
  translate([btn_x[1], sym_y]) mirror([0, 1]) sym_tri();       // PREV
  translate([btn_x[2], sym_y]) sym_cross();                    // BACK
  translate([btn_x[3], sym_y]) sym_check();                    // SELECT
  translate([icsp_c[0], 15.2]) sym_pins();                     // ICSP
  translate([24.6, 7.2]) text("TAPUINO", size = 4.2, font = "Liberation Sans:style=Bold", halign = "center", valign = "center", spacing = 1.1);
}
module lid_bed2d() {                               // contact area of the lid on the print bed
  difference() {
    outline();
    btn_recess2d();
    translate(win_c) square([win_s[0] + 2 * win_ch, win_s[1] + 2 * win_ch], center = true);
    translate(icsp_c) square(icsp_s, center = true);
    if (labels) labels2d();
  }
}

module ridge(len, h) {                             // triangular snap ridge along Y (45 deg flanks)
  rotate([-90, 0, 0]) linear_extrude(len) polygon([[0, h + 0.2], [0, -h - 0.2], [h, 0]]);
}
snap_y = [[5, 9], [35, 7]];                        // [start, length]
snap_z = zs + lap_h / 2;

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
        translate([x0, 0.5]) square([gap + 1.0, 17.0]);
        translate([x0, 34.5]) square([gap + 1.0, y1 - 34.5]);
        translate([x1 - gap - 1.0, 0.5]) square([gap + 1.0, y1 - 0.5]);
        translate([x0, pcb_y - 1.0]) square([x1 - x0, gap + 1.0]);
      }
    }
    bed_chamfer(zf - floor_t, 1) outline();
    // MicroSD slot (open towards the top, so no bridge)
    translate([ox0 - 1, sd_y[0], -sd_z]) cube([wall + 1 + gap + 0.2, sd_y[1] - sd_y[0], 20]);
    // snap grooves
    for (s = snap_y) {
      translate([ox0 + hw + e, s[0] - 0.3, snap_z]) mirror([1, 0, 0]) ridge(s[1] + 0.6, snap + 0.1);
      translate([ox1 - hw - e, s[0] - 0.3, snap_z]) ridge(s[1] + 0.6, snap + 0.1);
    }
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
      // inner halves of the side walls + button-side wall down to the board
      translate([0, 0, zs + 0.1]) linear_extrude(lap_h + e) difference() {
        intersection() { outline(); translate([ix0, 0]) square([ix1 - ix0, oy1]); }
        cavity2d();
      }
      // snap ridges
      for (s = snap_y) {
        translate([ix0 + e, s[0], snap_z]) mirror([1, 0, 0]) ridge(s[1], snap + fit);
        translate([ix1 - e, s[0], snap_z]) ridge(s[1], snap + fit);
      }
      // end curtain above the cassette connector
      translate([x0 - e, 0, zs + 4.1]) cube([x1 - x0 + 2 * e, 1.6, zc - zs - 4.1 + e]);
      // corner blocks: pull-out stop in front of the ICSP header (left) + hold-down (right)
      translate([x0 - e, 0.2, zs + 0.2]) cube([7.0 + gap, stop_y - 0.2, front_h]);
      translate([x1 - 7.0 - gap + e, 0.2, zs + 0.2]) cube([7.0 + gap, stop_y - 0.2, front_h]);
      // hold-down ribs on the board edges
      for (y = [14, 35.5]) {
        translate([x0 - e, y, zs + 0.15]) cube([gap + 1.2, 2, front_h]);
        translate([x1 - gap - 1.2 + e, y, zs + 0.15]) cube([gap + 1.2, 2, front_h]);
      }
    }
    // button cut-outs (open towards the joint)
    if (btn_teeth) for (x = btn_x) translate([x - btn_w / 2, pcb_y - 1.0, zs - 1]) cube([btn_w, wall_b + gap + 3, front_h + 1]);
    else translate([btn_x[0] - btn_w / 2, pcb_y - 1.0, zs - 1]) cube([btn_x[3] - btn_x[0] + btn_w, wall_b + gap + 3, front_h + 1]);
    // OLED window with bezel chamfer
    translate([win_c[0], win_c[1], zc - e]) hull() {
      translate([0, 0, 0]) linear_extrude(e) square(win_s, center = true);
      translate([0, 0, top_t]) linear_extrude(2 * e) square([win_s[0] + 2 * win_ch, win_s[1] + 2 * win_ch], center = true);
    }
    // ICSP opening
    translate([icsp_c[0] - icsp_s[0] / 2, icsp_c[1] - icsp_s[1] / 2, zc - 1]) cube([icsp_s[0], icsp_s[1], top_t + 2]);
    // lid ends at the board edge above the buttons
    translate([0, 0, zc - 1]) linear_extrude(top_t + 2) btn_recess2d();
    // lettering (debossed)
    if (labels) translate([0, 0, zc + top_t - label_d]) linear_extrude(label_d + e) labels2d();
    bed_chamfer(zc + top_t, -1) lid_bed2d();
  }
}

// ----------------------------------------------------------------------------- board dummy (fit check)
module board() {
  color("green") cube([pcb_x, pcb_y, pcb_t]);
  color("silver") for (x = btn_x) translate([x - 3.75, pcb_y - 6.55, pcb_t]) cube([7.5, 6.55, 4.7]);             // switch bodies
  color("white")  for (x = btn_x) translate([x, pcb_y, pcb_t + 2.5]) rotate([-90, 0, 0]) cylinder(d = 3.4, h = 1.0);
  color("navy")   translate([8.89, 6.99, pcb_t]) cube([25.4, 27.93, 4.3]);                                      // OLED module
  color("black")  translate([1.27, 2.54, pcb_t]) cube([5.08, 7.62, 2.54]);                                      // ICSP housing
  color("gold")   for (x = [2.54, 5.08], y = [3.81, 6.35, 8.89]) translate([x - 0.32, y - 0.32, pcb_t]) cube([0.64, 0.64, 8.6]);
  color("silver") translate([35.7, 8.6, pcb_t]) cube([4.8, 11.5, 3.6]);                                         // crystal
  color("silver") translate([0.2, 18.9, -1.9]) cube([14.5, 14.5, 1.9]);                                         // MicroSD socket
  color("dimgray") translate([-5.0, 20.1, -1.5]) cube([15, 11.0, 1.0]);                                         // MicroSD card
  color("silver") translate([1.2, 1.2, -3.7]) cube([40.7, 43.2, 3.7 - 1.9 + e]);                                // rear side parts up to the optocoupler height
  color("blue")   translate([pcb_x / 2 - 23.25, -8, pcb_t / 2 - 4.6]) cube([46.5, 8, 9.2]);                     // edge connector flange
  color("blue")   translate([pcb_x / 2 - 16.5, -12.5, pcb_t / 2 - 4.6]) cube([33, 4.5, 9.2]);
}

if (part == "tray") translate([0, 0, back_h + floor_t]) tray();
else if (part == "lid") translate([0, 0, zc + top_t]) rotate([0, 180, 0]) translate([-pcb_x, 0, 0]) lid();
else if (part == "lidview") lid();
else if (part == "fitcheck") intersection() { union() { tray(); lid(); } board(); }
else if (part == "exploded") { color("gainsboro") tray(); board(); color("gainsboro") translate([0, 0, 14]) lid(); }
else { color("gainsboro") tray(); board(); color("gainsboro", 0.85) lid(); }
