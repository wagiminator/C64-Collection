udev
----

In order to access the xu1541 as a normal user on a udev linux
system (e.g. ubuntu), the device permissions have to be adjusted.
Since udev controls the creation and deletion of the device entries
for device dynamically being plugged in, udev has to be told how
to handle the xu1541.

To achieve this simply place the 45-opencbm-xu1541.rules file into the
/etc/udev/rules.d directory.

You'll now be able to access the xu1541 even as a non-root user.
