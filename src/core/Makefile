PACKAGENAME?=lshw

CXX=c++
INCLUDES=
DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\"
CXXFLAGS=-g -Wall $(INCLUDES) $(DEFINES) $(RPM_OPT_FLAGS)
LDFLAGS=
LDSTATIC=
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia-legacy.o scsi.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o usb.o sysfs.o display.o heuristics.o parisc.o cpufreq.o partitions.o blockio.o lvm.o ideraid.o pcmcia.o volumes.o
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

hw.o: hw.h osutils.h version.h options.h heuristics.h
main.o: hw.h print.h version.h options.h mem.h dmi.h cpuinfo.h cpuid.h
main.o: device-tree.h pci.h pcmcia-legacy.h ide.h scsi.h spd.h network.h
main.o: isapnp.h fb.h usb.h sysfs.h display.h parisc.h cpufreq.h ideraid.h
print.o: print.h hw.h options.h version.h osutils.h
mem.o: version.h mem.h hw.h
dmi.o: version.h dmi.h hw.h osutils.h
device-tree.o: version.h device-tree.h hw.h osutils.h
cpuinfo.o: version.h cpuinfo.h hw.h osutils.h
osutils.o: version.h osutils.h
pci.o: version.h pci.h hw.h osutils.h
version.o: version.h
cpuid.o: version.h cpuid.h hw.h
ide.o: version.h cpuinfo.h hw.h osutils.h cdrom.h disk.h heuristics.h
cdrom.o: version.h cdrom.h hw.h partitions.h
pcmcia-legacy.o: version.h osutils.h
scsi.o: version.h mem.h hw.h cdrom.h disk.h osutils.h heuristics.h sysfs.h
disk.o: version.h disk.h hw.h partitions.h
spd.o: version.h spd.h hw.h osutils.h
network.o: version.h network.h hw.h osutils.h sysfs.h heuristics.h
isapnp.o: version.h isapnp.h hw.h pnp.h
pnp.o: version.h pnp.h hw.h
fb.o: version.h fb.h hw.h
options.o: version.h options.h osutils.h
usb.o: version.h usb.h hw.h osutils.h heuristics.h
sysfs.o: version.h sysfs.h hw.h osutils.h
display.o: display.h hw.h
heuristics.o: version.h sysfs.h hw.h osutils.h
parisc.o: version.h device-tree.h hw.h osutils.h heuristics.h
cpufreq.o: version.h hw.h osutils.h
partitions.o: version.h partitions.h hw.h blockio.h lvm.h osutils.h
blockio.o: version.h blockio.h osutils.h
lvm.o: version.h lvm.h hw.h blockio.h osutils.h
ideraid.o: version.h cpuinfo.h hw.h osutils.h cdrom.h disk.h heuristics.h
