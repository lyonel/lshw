PACKAGENAME = lshw
export PACKAGENAME

all clean release install:
	$(MAKE) -C src $@
