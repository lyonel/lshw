PACKAGENAME?=lshw

CXX=c++
CC=cc
GTKINCLUDES=$(shell pkg-config gtk+-2.0 --cflags)
INCLUDES=-I../core $(GTKINCLUDES)
CXXFLAGS=-g -Wall -Os $(INCLUDES)
CFLAGS=$(CXXFLAGS)
GTKLIBS=$(shell pkg-config gtk+-2.0 --libs)
LIBS=-L../core -llshw $(GTKLIBS)
LDFLAGS=

OBJS = gtk-lshw.o interface.o support.o callbacks.o
SRCS = $(OBJS:.o=.c)

all: gtk-$(PACKAGENAME)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

interface.c support.c callbacks.c: gtk-lshw.glade gtk-lshw.gladep
	glade-2 -w gtk-lshw.glade

gtk-$(PACKAGENAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

install: all
	
clean:
	rm -f $(OBJS) gtk-$(PACKAGENAME)

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null


# DO NOT DELETE

gtk-lshw.o: interface.h support.h
interface.o: callbacks.h interface.h support.h
support.o: support.h
callbacks.o: callbacks.h interface.h support.h
