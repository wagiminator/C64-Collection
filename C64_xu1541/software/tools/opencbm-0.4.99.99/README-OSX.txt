Building OpenCBM on Mac OS X
----------------------------

MacPorts
--------

1. Install MacPorts from http://www.macports.org/

2. Setup a local ports repository with the OpenCBM port:
   
   > mkdir /Users/<username>/ports
   
   Edit /opt/local/etc/macports/sources.conf and add the following line first:
   
   file:///Users/<username>/ports

   > cd /Users/<username>/ports
   > mkdir -p emulators/opencbm
   > cp <opencbm>/opencbm/ports/MacPorts/Portfile emulators/opencbm/
   > portindex

  Creating software index in /Users/<cnvogelg>/ports
	Adding port emulators/opencbm

	Total number of ports parsed:	1 
	Ports successfully parsed:	1	 
	Ports failed:			0
	
3. Build and install the opencbm MacPorts package

   > sudo port install opencbm

4. OpenCBM is installed in /opt/local.
   Adjust /opt/local/etc/opencbm.conf to select default USB adapter

5. Done!
