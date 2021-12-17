Install development tools
-------------------------
sudo apt install build-essential linuxdoc-tools git libusb-dev libncurses5-dev libncursesw5-dev

Compile and install cc65 (use version 2.13.2)
---------------------------------------------
make -f make/gcc.mak
sudo make -f make/gcc.mak install

Compile and install OpenCBM (use version 0.4.99.99)
---------------------------------------------------
make -f LINUX/Makefile opencbm plugin-xu1541
sudo make -f LINUX/Makefile install install-plugin-xu1541
sudo cp ./xu1541/udev/45-opencbm-xu1541.rules /etc/udev/rules.d

Troubleshooting
---------------
add /usr/local/lib/ to /etc/ld.so.conf and execute ldconfig (as root)
sudo chmod 777 /dev/cbm
Avoid connecting USB-Cable to a front panel.

OpenCBM Testing
---------------
Test:           cbmctrl detect
Directory:      cbmctrl dir 8
Format disk:    cbmformat 8 GAMES,42
Format nextgen: cbmforng -c -x 8 SOFTWARE,24
Copy to disk:   d64copy --transfer=original "image.d64" 8
Copy from disk: d64copy --transfer=original 8 "foo.d64"
Copy file:      cbmcopy --quiet --no-progress --transfer=original -r 8 "foo" -output="foo.prg"

References:
-----------
cc65:    https://github.com/cc65/cc65
OpenCBM: https://github.com/OpenCBM/OpenCBM
