PACKAGENAME = lshw
export PACKAGENAME

all clean release install snapshot gui:
	$(MAKE) -C src $@
