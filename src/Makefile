PACKAGENAME:=lshw
export PACKAGENAME
SNAPSHOT=0.$(shell cat .timestamp)

SQLITE?=0

DESTDIR?=/
PREFIX?=/usr
SBINDIR=$(PREFIX)/sbin
MANDIR=$(PREFIX)/share/man
DATADIR=$(PREFIX)/share
INSTALL?=install -p
STRIP?=strip
export DESTDIR
export PREFIX
export SBINDIR
export MANDIR
export DATADIR
export SQLITE

CXX?=c++
INCLUDES=-I./core/
DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\"
CXXFLAGS=-g -Wall -g $(INCLUDES) $(DEFINES) $(RPM_OPT_FLAGS)
ifeq ($(SQLITE), 1)
	CXXFLAGS+= -DSQLITE $(shell pkg-config --cflags sqlite3)
endif
LDFLAGS=-L./core/ -g
ifneq ($(shell $(LD) --help 2| grep -- --as-needed), )
	LDFLAGS+= -Wl,--as-needed
endif
LDSTATIC=-static
LIBS=-llshw -lresolv
ifeq ($(SQLITE), 1)
	LIBS+= $(shell pkg-config --libs sqlite3)
endif

export CXXFLAGS
export LIBS
export LDFLAGS

DATAFILES = pci.ids usb.ids oui.txt manuf.txt

all: $(PACKAGENAME) $(PACKAGENAME).1 $(DATAFILES)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: core
core:
	+make -C core all

$(PACKAGENAME): core $(PACKAGENAME).o
	$(CXX) $(LDFLAGS) -o $@ $(PACKAGENAME).o $(LIBS)

.PHONY: po
po:
	+make -C po all

.PHONY: gui
gui: core
	+make -C gui all

.PHONY: nologo
nologo:
	cp -f gui/artwork/nologo/* gui/artwork/

.PHONY: static
static: $(PACKAGENAME)-static

$(PACKAGENAME)-static: core core/lib$(PACKAGENAME).a $(PACKAGENAME).o
	$(CXX) $(LDSTATIC) $(LDFLAGS) -o $@ $(PACKAGENAME).o $(LIBS)
	$(STRIP) $@

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
	wget -O $@ http://anonsvn.wireshark.org/wireshark/trunk/manuf

install: all
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(SBINDIR)
	$(INSTALL) -m 0755 $(PACKAGENAME) $(DESTDIR)/$(SBINDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(MANDIR)/man1
	$(INSTALL) -m 0644 $(PACKAGENAME).1 $(DESTDIR)/$(MANDIR)/man1
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)
	$(INSTALL) -m 0644 $(DATAFILES) $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)
	make -C po install

install-gui: gui
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(SBINDIR)
	$(INSTALL) -m 0755 gui/gtk-$(PACKAGENAME) $(DESTDIR)/$(SBINDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)/artwork
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)/ui
	$(INSTALL) -m 0644 gui/*.ui $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)/ui
	$(INSTALL) -m 0644 gui/artwork/*.svg $(DESTDIR)/$(DATADIR)/$(PACKAGENAME)/artwork
	
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
	svn copy ${PWD} `dirname ${PWD}`/releases/`cat ../.version`
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
