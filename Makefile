PACKAGENAME = lshw
export PACKAGENAME

version.cpe: .version
	echo -n cpe:/a:ezix:$(PACKAGENAME): > $@
	cat $^ >> $@

all clean release install snapshot gui install-gui:
	+$(MAKE) -C src $@
