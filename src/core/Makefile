PACKAGENAME?=lshw

CXX=c++
INCLUDES=
DEFINES=-DPREFIX=\"$(PREFIX)\" -DSBINDIR=\"$(SBINDIR)\" -DMANDIR=\"$(MANDIR)\" -DDATADIR=\"$(DATADIR)\"
CXXFLAGS?=-g -Wall $(INCLUDES) $(DEFINES) $(RPM_OPT_FLAGS)
LDFLAGS=
LDSTATIC=
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia-legacy.o scsi.o dasd.o disk.o spd.o network.o isapnp.o pnp.o fb.o options.o usb.o sysfs.o display.o heuristics.o parisc.o cpufreq.o partitions.o blockio.o lvm.o ideraid.o pcmcia.o volumes.o mounts.o smp.o abi.o jedec.o dump.o fat.o virtio.o
ifeq ($(SQLITE), 1)
	OBJS+= db.o
endif
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

hw.o: hw.h osutils.h version.h config.h options.h heuristics.h
main.o: hw.h print.h version.h options.h mem.h dmi.h cpuinfo.h cpuid.h
main.o: device-tree.h pci.h pcmcia.h pcmcia-legacy.h ide.h scsi.h spd.h
main.o: network.h isapnp.h fb.h usb.h sysfs.h display.h parisc.h cpufreq.h
main.o: ideraid.h mounts.h smp.h abi.h dasd.h virtio.h
print.o: print.h hw.h options.h version.h osutils.h config.h
mem.o: version.h config.h mem.h hw.h
dmi.o: version.h config.h dmi.h hw.h osutils.h
device-tree.o: version.h device-tree.h hw.h osutils.h
cpuinfo.o: version.h cpuinfo.h hw.h osutils.h
osutils.o: version.h osutils.h
pci.o: version.h config.h pci.h hw.h osutils.h options.h
version.o: version.h config.h
cpuid.o: version.h cpuid.h hw.h
ide.o: version.h cpuinfo.h hw.h osutils.h cdrom.h disk.h heuristics.h
cdrom.o: version.h cdrom.h hw.h partitions.h
pcmcia-legacy.o: version.h pcmcia-legacy.h hw.h osutils.h
scsi.o: version.h mem.h hw.h cdrom.h disk.h osutils.h heuristics.h sysfs.h
disk.o: version.h disk.h hw.h osutils.h heuristics.h partitions.h
spd.o: version.h spd.h hw.h osutils.h
network.o: version.h config.h network.h hw.h osutils.h sysfs.h options.h
network.o: heuristics.h
isapnp.o: version.h isapnp.h hw.h pnp.h
pnp.o: version.h pnp.h hw.h
fb.o: version.h fb.h hw.h
options.o: version.h options.h osutils.h
usb.o: version.h usb.h hw.h osutils.h heuristics.h options.h
sysfs.o: version.h sysfs.h hw.h osutils.h
display.o: display.h hw.h
heuristics.o: version.h sysfs.h hw.h osutils.h
parisc.o: version.h device-tree.h hw.h osutils.h heuristics.h
cpufreq.o: version.h hw.h osutils.h
partitions.o: version.h partitions.h hw.h blockio.h lvm.h volumes.h osutils.h
blockio.o: version.h blockio.h osutils.h
lvm.o: version.h lvm.h hw.h blockio.h osutils.h
ideraid.o: version.h cpuinfo.h hw.h osutils.h cdrom.h disk.h heuristics.h
pcmcia.o: version.h pcmcia.h hw.h osutils.h sysfs.h
volumes.o: version.h volumes.h hw.h blockio.h lvm.h osutils.h
mounts.o: version.h mounts.h hw.h osutils.h
smp.o: version.h smp.h hw.h osutils.h
abi.o: version.h abi.h hw.h osutils.h
jedec.o: jedec.h
dasd.o: dasd.h disk.h hw.h osutils.h
virtio.o: version.h hw.h sysfs.h disk.h virtio.h
