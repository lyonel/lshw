PACKAGENAME:=lshw
export PACKAGENAME
SNAPSHOT=0.`cat .timestamp`

DESTDIR=/
PREFIX=/usr
SBINDIR=$(PREFIX)/sbin
MANDIR=$(PREFIX)/share/man
DATADIR=$(PREFIX)/share/$(PACKAGENAME)
export DESTDIR
export PREFIX
export SBINDIR
export MANDIR
export DATADIR

CXX=c++
INCLUDES=-I./core/
CXXFLAGS=-g -Wall -Os $(INCLUDES)
LDFLAGS=-L./core/
LDSTATIC=-static
LIBS=-llshw

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia.o scsi.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o lshw.o usb.o sysfs.o display.o heuristics.o
SRCS = $(OBJS:.o=.cc)

DATAFILES = pci.ids

all: $(PACKAGENAME) $(PACKAGENAME).1 $(DATAFILES)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

core/lib$(PACKAGENAME).a:
	make -C core all

$(PACKAGENAME): core/lib$(PACKAGENAME).a $(PACKAGENAME).o
	$(CXX) $(LDFLAGS) -o $@ $(LIBS) $(PACKAGENAME).o

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
	rm -f $(PACKAGENAME).o $(PACKAGENAME) $(PACKAGENAME)-static $(PACKAGENAME)-compressed
	make -C core clean

.timestamp:
	date --utc +%Y%m%d%H%M%S > $@
                                                                               
release:
	mkdir -p ../../releases
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


