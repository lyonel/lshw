PACKAGENAME?=lshw

CXX=c++
INCLUDES=
CXXFLAGS=-g -Wall -Os $(INCLUDES)
LDFLAGS=
LDSTATIC=
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia.o scsi.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o usb.o sysfs.o display.o heuristics.o
SRCS = $(OBJS:.o=.cc)

all: lib$(PACKAGENAME).a

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib$(PACKAGENAME).a: $(OBJS)
	$(AR) rs $@ $^

install: all
	
clean:
	rm -f $(OBJS) lib$(PACKAGENAME).a

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null


