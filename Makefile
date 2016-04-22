PACKAGENAME = lshw
VERSION?= $(shell git describe --tags --long | cut -d - -f 1,2 | tr - .)
export PACKAGENAME

all clean install snapshot gui install-gui:
	+$(MAKE) -C src $@

version.cpe: .version
	echo -n cpe:/a:ezix:$(PACKAGENAME): > $@
	cat $^ >> $@

.PHONY: $(PACKAGENAME).spec

$(PACKAGENAME).spec: $(PACKAGENAME).spec.in
	cat $^ | sed -e s/\@VERSION\@/$(VERSION)/g > $@

release: $(PACKAGENAME).spec
	git archive --prefix=$(PACKAGENAME)-$(VERSION)/ -o $(PACKAGENAME)-$(VERSION).tar HEAD
	tar -rf $(PACKAGENAME)-$(VERSION).tar $^
	gzip -f $(PACKAGENAME)-$(VERSION).tar
