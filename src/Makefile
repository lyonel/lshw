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

DATAFILES = pci.ids usb.ids oui.txt manuf.txt

all: $(PACKAGENAME) $(PACKAGENAME).1 $(DATAFILES)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: core
core:
	+make -C core all

$(PACKAGENAME): core $(PACKAGENAME).o
	$(CXX) $(LDFLAGS) -o $@ $(PACKAGENAME).o $(LIBS) 

.PHONY: gui
gui: core
	+make -C gui all

.PHONY: static
static: $(PACKAGENAME)-static

$(PACKAGENAME)-static: core/lib$(PACKAGENAME).a $(PACKAGENAME).o
	$(CXX) $(LDSTATIC) $(LDFLAGS) -o $@ $(PACKAGENAME).o $(LIBS)
	strip $@

.PHONY: compressed
compressed: $(PACKAGENAME)-compressed

$(PACKAGENAME)-compressed: $(PACKAGENAME)-static
	upx -9 -o $@ $<

$(PACKAGENAME).1: $(PACKAGENAME).sgml
	docbook2man $<

pci.ids:
	wget http://pciids.sourceforge.net/pci.ids

usb.ids:
	wget http://www.linux-usb.org/usb.ids

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

install-gui: gui
	-mkdir -p $(DESTDIR)
	-mkdir -p $(DESTDIR)/$(SBINDIR)
	cp gui/gtk-$(PACKAGENAME) $(DESTDIR)/$(SBINDIR)
	
clean:
	rm -f $(PACKAGENAME).o $(PACKAGENAME) $(PACKAGENAME)-static $(PACKAGENAME)-compressed
	make -C core clean
	make -C gui clean

.timestamp:
	date --utc +%Y%m%d%H%M%S > $@

$(PACKAGENAME).spec: $(PACKAGENAME).spec.in
	cat $^ | sed -e "s/\@VERSION\@/`cat ../.version`/g" > $@
                                                                               
release: $(PACKAGENAME).spec
	mkdir -p ../../releases
	svn copy .. `dirname ${PWD}`/releases/`cat ../.version`
	svn commit `dirname ${PWD}`/releases/`cat ../.version` -m "released version "`cat ../.version`" of "$(PACKAGENAME)
	rm -rf $(PACKAGENAME)-`cat ../.version`
	svn export ../../releases/`cat ../.version` $(PACKAGENAME)-`cat ../.version`
	cp $^ $(PACKAGENAME)-`cat ../.version`/$(PACKAGENAME).spec
	tar cfz $(PACKAGENAME)-`cat ../.version`.tar.gz --owner=0 --group=0 --numeric-owner --mode="go-w" $(PACKAGENAME)-`cat ../.version`
	rm -rf $(PACKAGENAME)-`cat ../.version`

snapshot: .timestamp
	rm -rf $(PACKAGENAME)-$(SNAPSHOT)
	svn export -r HEAD .. $(PACKAGENAME)-$(SNAPSHOT)
	cat $(PACKAGENAME)-$(SNAPSHOT)/src/$(PACKAGENAME).spec.in | sed -e "s/\@VERSION\@/$(SNAPSHOT)/g" > $(PACKAGENAME)-$(SNAPSHOT)/$(PACKAGENAME).spec
	tar cfz $(PACKAGENAME)-$(SNAPSHOT).tar.gz --owner=0 --group=0 --numeric-owner --mode="go-w" $(PACKAGENAME)-$(SNAPSHOT)
	rm -rf $(PACKAGENAME)-$(SNAPSHOT)
	rm -f .timestamp

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null


# DO NOT DELETE
