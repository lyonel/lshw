PACKAGENAME = lshw
export PACKAGENAME

all clean release install snapshot gui install-gui:
	+$(MAKE) -C src $@

version.cpe: .version
	echo -n cpe:/a:ezix:$(PACKAGENAME): > $@
	cat $^ >> $@

