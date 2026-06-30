.PHONY: all release clean clean-release

all:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu
	$(MAKE) -f ${CURDIR}/../Makefile.pal

release:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu release
	$(MAKE) -f ${CURDIR}/../Makefile.pal release

clean:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu clean
	$(MAKE) -f ${CURDIR}/../Makefile.pal clean
	rm -f *.bin *.elf

clean-release:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu clean-release
	$(MAKE) -f ${CURDIR}/../Makefile.pal clean-release
