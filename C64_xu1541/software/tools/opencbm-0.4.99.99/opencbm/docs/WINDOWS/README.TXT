cbm4win 0.1.0 README

There is a web page available for cbm4win. Get updates from there:

  http://sf.net/projects/cbm4win/


1. Supported operating system (OS)

   First, you have to make sure that you are running a suitable OS version. You
   have to run one of the following operating systems:

   - Windows NT 4.0
   - Windows 2000
   - Windows XP

   It might be possible to run the driver on Windows NT 3.1, NT 3.5, NT 3.51.
   Anyway, as I do not have any of these, I cannot test it. If you have one of
   these running, feel free to test, and report back if it works or not.

   Specifically, the following OS versions are not suitable:

   - Windows 95
   - Windows 98
   - Windows Me (Millenium Edition)

   In fact, OpenCBM will refuse to load on one of these operating systems.
   Anyway, support for Windows 98 (Second Edition) and Me is planned.


2. Needed equipment

   OpenCBM needs an XA1541 active cable or an XM1541 multitasking cable to
   work.  In fact, only the XA1541 cable has been tested.

   Furthermore, it needs a CBM serial bus drive, for example, a CBM 1540, CBM
   1541, a CBM 1570, or a CBM 1571. So far, only 1541 and 1571 have been
   tested.

   If your drive is equipped with a parallel cable, you will find it suitable
   to use the XAP1541 adaptor, or similar approaches to get a XP1541 cable for
   connecting your drive to the PC. This speeds up transfer times considerably.


3. Development issues

   THIS DRIVER IS IN DEVELOPMENT! BECAUSE OF THIS, I DO NOT RECOMMEND TO USE
   IT! USE AT YOUR OWN RISK!

   As this is a kernel-mode driver, malfunctioning can cause severe damage to
   your installed operating system and any application and/or data on it.
   Because of this, you should not use this driver without DOING A BACKUP
   BEFORE STARTING THIS DRIVER!


4. Acknowledgements, thanks and links

   Cbm4win is heavily based on cbm4linux, written by Michael Klein. Cbm4linux
   itself uses work from Star Commander, written by KOVÁCS Balázs a.k.a. "Joe
   Forster". I like to thank both for their steady help and their patience.

   Furthermore, I want to thank the following people:
   - Michael Klein for cbm4linux and many discussions. I do not know if I would
     have started this project without this.
   - KOVÁCS Balázs a.k.a. "Joe Forster" for Star Commander, his advice, and for
     giving me cables for free to work on this project
   - Wolfgang Moser for steady discussions, testing, and bug reports, as well
     as giving me some more needed equipment;
   - all of my testers for steady tests, reports, and discussions.



5. Contact, bug tracking, mailing lists

   Feel free to contact me if you have questions, suggestions, or you just want
   to confirm that something does work or does not. Specifically, if you are
   using cables not specified above, or drives not specified above, I would
   like to hear if OpenCBM works with this combo.

   More specifically, I have setup a project page on sourceforge:

              http://sf.net/projects/cbm4win/
   
   If you want to submit bug reports, I would thank you for submitting to
   the bug tracking system over there. There are even two mailing lists
   available for announcements and for user discussions.

   If you want to contact me directly, just contact me at
   cbm4win(at)trikaliotis(dot)net, or go to my homepage at
   http://www.trikaliotis.net/.
