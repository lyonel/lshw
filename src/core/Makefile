PACKAGENAME?=lshw

CXX=c++
INCLUDES=
DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\" -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CXXFLAGS=-g -Wall -Os $(INCLUDES) $(DEFINES)
LDFLAGS=
LDSTATIC=
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia.o scsi.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o usb.o sysfs.o display.o heuristics.o parisc.o cpufreq.o partitions.o
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
ide.o: cpuinfo.h hw.h osutils.h cdrom.h disk.h heuristics.h
cdrom.o: cdrom.h hw.h
pcmcia.o: pcmcia.h hw.h osutils.h
scsi.o: mem.h hw.h cdrom.h disk.h osutils.h heuristics.h
disk.o: disk.h hw.h
spd.o: spd.h hw.h osutils.h
network.o: network.h hw.h osutils.h sysfs.h
isapnp.o: isapnp.h hw.h pnp.h
pnp.o: pnp.h hw.h
fb.o: fb.h hw.h
options.o: options.h osutils.h
usb.o: usb.h hw.h osutils.h heuristics.h
sysfs.o: sysfs.h hw.h osutils.h
display.o: display.h hw.h
heuristics.o: sysfs.h hw.h osutils.h
