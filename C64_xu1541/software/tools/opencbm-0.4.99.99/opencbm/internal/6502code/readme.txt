This directory contains an archive with the 6502 assembler sources files
for all the custom drive routines that are used by The Star Commander.
Joe Forster, author of The Star Commander officially provided these to
the OpenCBM project in a licensing discussion on the OpenCBM developer
mailing list from 2009-01-29 to 2009-02-08.

The archive as well as Joe's mail are stored here to document this step.

Early versions of cbm4linux and OpenCBM contained 6502 assembler code
routines derived from the Star Commander's routines upon a personal
agreement between Michael Klein and Joe Forster. These codes were put
under the GPL with a special agreement to allow for reintegration of
patches into the Star Commander.
One of the goals of the discussion held from 2009-01-29 to 2009-02-08
was to simplify the licensing and code management. Reintegration of code
parts into the Star Commander is implictly allowed by the chosen license
now (modified BSD license).

The assembler source codes in this archive are not 100% identical to the
ones used in OpenCBM. Historically Joe Forster always used a machine
language monitor to edit 6502 code directly in memory. In 1999 Michael
Klein translated needed parts (mainly the turbo protocols) into 6502
assembler for the XA cross assembler. In 2004 Joe Forster himself
reverse engineered all of the Star Commander 6502 routines into
assembler code and also did a great job in commenting the source.
