PACKAGENAME = lshw
export PACKAGENAME

all clean release install snapshot gui install-gui:
	$(MAKE) -C src $@
