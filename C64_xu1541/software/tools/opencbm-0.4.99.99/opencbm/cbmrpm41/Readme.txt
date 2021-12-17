The high precision 1541 RPM measurement tool for OpenCBM

CBMrpm41 is a rewrite from scratch of the former sample project
rpm1541. It bases on a unique and new developed technique to do
time measurements on a Commodore 1541 floppy disk drive.

  Code: (C) 2006 Wolfgang Moser

Licenses:
  Host program: GNU General Public License
  6502 code:    modified BSD license (the 3 clause BSD license)

Some of the features implemented in the rewritten  6502  routine:

* VIA shift register usage (the core feature), but not the shift
  register itself is used but the 8 bit latchable timer that drives
  the shift register's clock
* VIA bugs regarding the shift register and more extracts from the
  former "viatimers.a65" testproject
* virtual 24 Bit timer construction
* Chinese Remainder Theorem, Extended Euklidean Algorithm and
  Coefficients calculation as well as determining the "best" modulus
  (latch register reload value)
* Tested shift register modes 101, 100 and finally 001 and their
  drawbacks
* Software extension of the virtual 23.598 bits timer to fully 32
  bits
* Ux command table programming with a user defined table at $0306
  (maybe as an idea for the on-demand o65 linker)
* Using ASCII-2-HEX (PETSCII ?) conversion routines in the 1541
  for parameter passing instead of transferring plain bytes
* For RPM measurements the block head to look for is measured out
  with a jitter of 0...1 regarding synchronization to the Byte
  Ready signal. The jitter is 0...2 with a standard  "bvc *"  loop,
  by precisely adjusting a so named "sync in" area, the jitter
  could be reduced by one.


* Possible applications for the high precision timer system
    + measuring out track-to-track alignment (done as job ID 2)
    + measuring out on-track sector distances
    + measuring out on-track SYNC lengths as well as distances
    + measuring mean bitrates
    + measuring sector distances (on-track) and sector distribution


Wolfgang Moser, 2009-02-10
