PACKAGENAME?=lshw

CXX=c++
CC=cc

DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\"
GTKINCLUDES=$(shell pkg-config gtk+-2.0 --cflags)
INCLUDES=-I../core $(GTKINCLUDES)
CXXFLAGS=-g -Wall -Os $(INCLUDES) $(DEFINES)
CFLAGS=$(CXXFLAGS) $(DEFINES)
GTKLIBS=$(shell pkg-config gtk+-2.0 --libs)
LIBS=-L../core -llshw $(GTKLIBS)
LDFLAGS= -Os -s
ifneq (`ld --help 2| grep -- --as-needed`, '')
	LDOPTIMIZE=-Wl,--as-needed
endif

OBJS = gtk-lshw.o interface.o support.o callbacks.o engine.o print.o stock.o
SRCS = $(OBJS:.o=.c)

all: gtk-$(PACKAGENAME)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

interface.c: gtk-lshw.glade gtk-lshw.gladep
	glade-2 -w gtk-lshw.glade

gtk-$(PACKAGENAME): $(OBJS) ../core/liblshw.a
	$(CXX) $(LDFLAGS) $(LDOPTIMIZE) -o $@ $^ $(LIBS)

install: all
	strip gtk-$(PACKAGENAME)
	
clean:
	rm -f $(OBJS) gtk-$(PACKAGENAME) gtk-lshw.glade.bak gtk-lshw.gladep.bak interface.c.bak interface.h.bak callbacks.c.bak callbacks.h.bak Makefile.bak

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null


# DO NOT DELETE

gtk-lshw.o: interface.h support.h
interface.o: callbacks.h interface.h support.h
support.o: support.h
callbacks.o: callbacks.h interface.h support.h
