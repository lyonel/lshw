PACKAGENAME = lshw
export PACKAGENAME

all clean release install snapshot:
	$(MAKE) -C src $@
