lshw: HardWare LiSter for Linux
===============================

lshw is a small tool to provide detailed information on the hardware configuration of the machine. It can report exact memory configuration, firmware version, mainboard configuration, CPU version and speed, cache configuration, bus speed, etc. on DMI-capable x86 or EFI (IA-64) systems and on some ARM and PowerPC machines (PowerMac G4 is known to work).

Information can be output in plain text, XML or HTML.

It currently supports DMI (x86 and EFI only), OpenFirmware device tree
(PowerPC only), PCI/AGP, ISA PnP (x86), CPUID (x86), IDE/ATA/ATAPI, PCMCIA
(only tested on x86), USB and SCSI.

Installation
------------

 1. Requirements
   - Linux 2.4.x, 2.6.x or 3.x (2.2.x might work, though)
   - a PA-RISC-, Alpha-, IA-64- (Itanium-), PowerPC-, ARM- or x86- based machine
   - an ANSI (or close enough to ANSI compliance) C++ compiler (tested with g++ 2.95.4 and 3.x)
   - for the (optional) GTK+ graphical user interface, you will need a
	complete GTK+ development environment (gtk2-devel on RedHat/Fedora derivatives) 

 2. To compile it, just use:

    	$ make

 3. If you want to build the optional GUI, do:

    	$ make
    	$ make gui

 4. the lshw home page is http://lshw.ezix.org/
 5. send bug reports, requests for help, feature requests, comments, etc. to bugs@ezix.org.  The author can be contacted directly (lyonel@ezix.org)
 
   Please make sure you include enough information in your bug report: XML output from lshw is preferred over text or HTML, indicate the affected version of lshw, your platform (i386, x86-64, PA-RISC, PowerPC, etc.) and your distribution.

NOTE TO DISTRIBUTIONS
---------------------

By default, lshw includes its own lists of PCI IDs, USB IDs, etc. but will also look for this information in

    /usr/share/lshw/,
    /usr/local/share/,
	/usr/share/,
	/etc/,
	/usr/share/hwdata/,
	/usr/share/misc/

Statically-linked and/or compressed binaries can be built by using

    $ make static

or

    $ make compressed

in the `src/` directory

Building compressed binaries requires `upx` (cf. http://upx.sourceforge.net/).

