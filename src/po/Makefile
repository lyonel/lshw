PACKAGENAME?=lshw

CXX?=c++
CC?=cc
STRIP?=strip
OBJCOPY?=objcopy

LANGUAGES = fr
SRCS = $(LANGUAGES:=.po)
CATALOGS = $(LANGUAGES:=.mo)

all: ${PACKAGENAME}.pot ${CATALOGS}

.PHONY: ${PACKAGENAME}.pot
${PACKAGENAME}.pot:
	touch $@
	xgettext -F --no-wrap --indent --join-existing --keyword=_ -d ${PACKAGENAME} -o $@ ../*.cc ../core/*.cc ../gui/*.cc ../gui/*.c

%.mo: %.po
	msgfmt -o $@ $^

install: all
	
clean:
	rm -f $(OBJS) gtk-$(PACKAGENAME) gtk-lshw.glade.bak gtk-lshw.gladep.bak interface.c.bak interface.h.bak callbacks.c.bak callbacks.h.bak Makefile.bak
