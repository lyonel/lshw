PACKAGENAME:=lshw
SNAPSHOT=0.`cat .timestamp`

DESTDIR=/
PREFIX=/usr
SBINDIR=$(PREFIX)/sbin
MANDIR=$(PREFIX)/share/man
DATADIR=$(PREFIX)/share/$(PACKAGENAME)

CXX=c++
CXXFLAGS=-g -Wall -Os
LDFLAGS=
LDSTATIC=-static
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia.o scsi.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o lshw.o usb.o sysfs.o display.o
SRCS = $(OBJS:.o=.cc)

DATAFILES = pci.ids

all: $(PACKAGENAME) $(PACKAGENAME).1 $(DATAFILES)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PACKAGENAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(LIBS) $^

.PHONY: static
static: $(PACKAGENAME)-static

$(PACKAGENAME)-static: $(OBJS)
	$(CXX) $(LDSTATIC) $(LDFLAGS) -o $@ $(LIBS) $^
	strip $@

.PHONY: compressed
compressed: $(PACKAGENAME)-compressed

$(PACKAGENAME)-compressed: $(PACKAGENAME)-static
	upx -9 -o $@ $<

$(PACKAGENAME).1: $(PACKAGENAME).sgml
	docbook2man $<

pci.ids:
	wget http://pciids.sourceforge.net/pci.ids

oui.txt:
	wget http://standards.ieee.org/regauth/oui/oui.txt

manuf.txt:
	wget http://www.ethereal.com/distribution/manuf.txt

install: all
	-mkdir -p $(DESTDIR)
	-mkdir -p $(DESTDIR)/$(SBINDIR)
	cp $(PACKAGENAME) $(DESTDIR)/$(SBINDIR)
	strip $(DESTDIR)/$(SBINDIR)/$(PACKAGENAME)
	-mkdir -p $(DESTDIR)/$(MANDIR)/man1
	cp $(PACKAGENAME).1 $(DESTDIR)/$(MANDIR)/man1
	-mkdir -p $(DESTDIR)/$(DATADIR)
	cp $(DATAFILES) $(DESTDIR)/$(DATADIR)
	
clean:
	rm -f $(OBJS) $(PACKAGENAME) $(PACKAGENAME)-static $(PACKAGENAME)-compressed core

.timestamp:
	date --utc +%Y%m%d%H%M%S > $@
                                                                               
release:
	mkdir -p ../releases
	svn copy .. `dirname ${PWD}`/releases/`cat ../.version`
	svn commit `dirname ${PWD}`/releases/`cat ../.version` -m "released version "`cat ../.version`" of "$(PACKAGENAME)
	rm -rf $(PACKAGENAME)-`cat ../.version`
	svn export ../../releases/`cat ../.version` $(PACKAGENAME)-`cat ../.version`
	cat $(PACKAGENAME)-`cat ../.version`/src/$(PACKAGENAME).spec.in | sed -e "s/\@VERSION\@/`cat ../.version`/g" > $(PACKAGENAME)-`cat ../.version`/$(PACKAGENAME).spec
	tar cfz $(PACKAGENAME)-`cat ../.version`.tar.gz $(PACKAGENAME)-`cat ../.version`
	rm -rf $(PACKAGENAME)-`cat ../.version`

snapshot: .timestamp
	rm -rf $(PACKAGENAME)-$(SNAPSHOT)
	svn export -r HEAD .. $(PACKAGENAME)-$(SNAPSHOT)
	cat $(PACKAGENAME)-$(SNAPSHOT)/src/$(PACKAGENAME).spec.in | sed -e "s/\@VERSION\@/$(SNAPSHOT)/g" > $(PACKAGENAME)-$(SNAPSHOT)/$(PACKAGENAME).spec
	tar cfz $(PACKAGENAME)-$(SNAPSHOT).tar.gz $(PACKAGENAME)-$(SNAPSHOT)
	rm -rf $(PACKAGENAME)-$(SNAPSHOT)
	rm -f .timestamp

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null

# DO NOT DELETE

hw.o: hw.h osutils.h
main.o: hw.h print.h version.h options.h mem.h dmi.h cpuinfo.h cpuid.h
main.o: device-tree.h pci.h pcmcia.h ide.h scsi.h spd.h network.h isapnp.h
main.o: fb.h usb.h sysfs.h display.h
print.o: print.h hw.h options.h version.h osutils.h
mem.o: mem.h hw.h
dmi.o: dmi.h hw.h osutils.h
device-tree.o: device-tree.h hw.h osutils.h
cpuinfo.o: cpuinfo.h hw.h osutils.h
osutils.o: osutils.h
pci.o: pci.h hw.h osutils.h
version.o: version.h
cpuid.o: cpuid.h hw.h
ide.o: cpuinfo.h hw.h osutils.h cdrom.h disk.h
cdrom.o: cdrom.h hw.h
pcmcia.o: pcmcia.h hw.h osutils.h
scsi.o: mem.h hw.h cdrom.h disk.h osutils.h
disk.o: disk.h hw.h
spd.o: spd.h hw.h osutils.h
network.o: network.h hw.h osutils.h sysfs.h
isapnp.o: isapnp.h hw.h pnp.h
pnp.o: pnp.h hw.h
fb.o: fb.h hw.h
options.o: options.h osutils.h
lshw.o: hw.h print.h main.h version.h options.h
usb.o: usb.h hw.h osutils.h
sysfs.o: sysfs.h hw.h osutils.h
display.o: fb.h hw.h
