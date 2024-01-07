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
   - Linux 2.4.x, 2.6.x, 3.x or 4.x (2.2.x might work, though)
   - a PA-RISC-, Alpha-, IA-64- (Itanium-), PowerPC-, ARM- or x86- based machine
   - cmake, GNU make or Ninja
   - an ANSI (or close enough to ANSI compliance) C++ compiler (tested with g++ 2.95.4 and 3.x)
   - for the (optional) GTK+ graphical user interface, you will need a
	complete GTK+ development environment (gtk3-devel on RedHat/Fedora derivatives)
   - for optional SQLite feature install SQLite
   - for optional zlib feature install zlib and gzip

 2. Use cmake options to decide feature set:
   - -DGUI=OFF - disable graphical user interface version og lshw
   - -DZLIB=ON - enable reading of gzipped datafiles
   - -DSQLITE=ON -  enable SQLite support
   - -DPOLICYKIT=ON - enable PolicyKit integration
   - -DNOLOGO=ON - don't install logos with copyright

 3. Do configuration and build by

       $ mkdir build && cd build
       $ cmake .. -GNinja <options>
       $ ninja-build

 4. If you want to install the result, do:

       $ ninja-build install

Getting help
------------

 1. the lshw home page is http://lshw.ezix.org/
 2. bug reports and feature requests: http://ezix.org/project/newticket?component=lshw

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

If compiled with zlib support, lshw will look for `file`.gz first, then for `file`.

Statically-linked and/or compressed binaries can be built by using

    $ mkdir build && cd build
    $ cmake .. -DSTATIC=ON
    $ ninja
or
    $ mkdir build && cd build
    $ cmake .. -GNinja
    $ ninja compressed

Building compressed binaries requires `upx` (cf. https://upx.github.io/).

Release management and data files maintenance
---------------------------------------------

Create release tarball,

 1. Edit CMakeLists.txt to set version
 2. Run
    $ mkdir build && cd build
    $ cmake .. -DMAKE_RELEASE=ON
    $ make release

Update hwdata files:

    $ make refresh_hwdata
