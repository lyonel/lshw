PACKAGENAME = lshw

CXX=c++
CXXFLAGS=-g
LDFLAGS=
LIBS=

OBJS = hw.o main.o print.o mem.o dmi.o device-tree.o cpuinfo.o osutils.o pci.o version.o cpuid.o ide.o cdrom.o pcmcia.o scsi.o disk.o
SRCS = $(OBJS:.o=.cc)

all: $(PACKAGENAME) $(PACKAGENAME).1

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PACKAGENAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(LIBS) $^

$(PACKAGENAME).1: $(PACKAGENAME).sgml
	docbook2man $<
	
clean:
	rm -f $(OBJS) $(PACKAGENAME) core

.tag: .version
	cat $< | sed -e 'y/./_/' > $@
                                                                               
release: .tag
	cvs tag -cF `cat .tag`
	rm -rf $(PACKAGENAME)-`cat .version`
	cvs export -r `cat .tag` -d $(PACKAGENAME)-`cat .version` `cat CVS/Repository`
	tar cfz $(PACKAGENAME)-`cat .version`.tar.gz $(PACKAGENAME)-`cat .version`
	rm -rf $(PACKAGENAME)-`cat .version`

depend:
	@makedepend -Y $(SRCS) 2> /dev/null > /dev/null

# DO NOT DELETE

hw.o: hw.h osutils.h
main.o: hw.h print.h version.h mem.h dmi.h cpuinfo.h cpuid.h device-tree.h
main.o: pci.h pcmcia.h ide.h scsi.h
print.o: print.h hw.h version.h osutils.h
mem.o: mem.h hw.h
dmi.o: dmi.h hw.h
device-tree.o: device-tree.h hw.h osutils.h
cpuinfo.o: cpuinfo.h hw.h osutils.h
osutils.o: osutils.h
pci.o: pci.h hw.h osutils.h
version.o: version.h
cpuid.o: cpuid.h hw.h
ide.o: cpuinfo.h hw.h osutils.h cdrom.h disk.h
cdrom.o: cdrom.h hw.h
pcmcia.o: pcmcia.h hw.h osutils.h
scsi.o: mem.h hw.h cdrom.h disk.h osutils.h
disk.o: disk.h hw.h
