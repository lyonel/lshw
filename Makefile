PACKAGENAME = lshw

CXX=c++
CXXFLAGS=-g
LDFLAGS=
LIBS=

OBJS = hw.o main.o print.o mem.o

all: $(PACKAGENAME)

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PACKAGENAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(LIBS) $^
	
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
