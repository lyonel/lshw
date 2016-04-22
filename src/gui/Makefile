PACKAGENAME?=lshw

CXX?=c++
CC?=cc
STRIP?=strip
OBJCOPY?=objcopy

DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\"
GTKINCLUDES=$(shell pkg-config gtk+-2.0 --cflags)
INCLUDES=-I../core $(GTKINCLUDES)
CXXFLAGS=-g -Wall $(INCLUDES) $(DEFINES) $(RPM_OPT_FLAGS)
CFLAGS=$(CXXFLAGS) $(DEFINES)
GTKLIBS=$(shell pkg-config gtk+-2.0 gmodule-2.0 --libs)
LIBS=-L../core -llshw -lresolv $(GTKLIBS)
LDFLAGS=
ifneq ($(shell $(LD) --help 2| grep -- --as-needed), )
	LDFLAGS+= -Wl,--as-needed
endif

OBJS = gtk-lshw.o callbacks.o engine.o print-gui.o stock.o
SRCS = $(OBJS:.o=.c)

all: gtk-$(PACKAGENAME)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: icon
icon: gtk-$(PACKAGENAME) artwork/logo.svg
	$(OBJCOPY) --add-section .icon=artwork/logo.svg gtk-$(PACKAGENAME)

interface.c: gtk-lshw.glade gtk-lshw.gladep
	glade-2 -w gtk-lshw.glade

gtk-$(PACKAGENAME): $(OBJS) ../core/liblshw.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

install: all
	$(STRIP) gtk-$(PACKAGENAME)
	
clean:
	rm -f $(OBJS) gtk-$(PACKAGENAME) gtk-lshw.glade.bak gtk-lshw.gladep.bak callbacks.c.bak callbacks.h.bak Makefile.bak

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null


# DO NOT DELETE

gtk-lshw.o: stock.h engine.h
callbacks.o: callbacks.h support.h engine.h
stock.o: stock.h
