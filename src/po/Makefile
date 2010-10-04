PACKAGENAME?=lshw

LANGUAGES = fr
SRCS = $(LANGUAGES:=.po)
CATALOGS = $(LANGUAGES:=.mo)

all: $(PACKAGENAME).pot $(CATALOGS)

POTFILES:
	find .. -name \*.cc > $@
	find .. -name \*.c >> $@

$(PACKAGENAME).pot: POTFILES
	xgettext -F --no-wrap --indent --keyword=_ --keyword=N_ -d $(PACKAGENAME) -o $@ -f POTFILES

%.mo: %.po
	msgfmt -v -o $@ $^

install: $(CATALOGS)
	$(foreach i, $(LANGUAGES), install -D $(i).mo $(DESTDIR)/$(DATADIR)/locale/$(i)/LC_MESSAGES/$(PACKAGENAME).mo ;)
	
clean:
	rm -f $(CATALOGS) $(PACKAGENAME).pot
