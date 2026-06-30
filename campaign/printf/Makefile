.PHONY: all release clean clean-release

all:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu
	$(MAKE) -f ${CURDIR}/../Makefile.pal
	$(MAKE) -f ${CURDIR}/../Makefile.ntscj

release:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu release
	$(MAKE) -f ${CURDIR}/../Makefile.pal release
	$(MAKE) -f ${CURDIR}/../Makefile.ntscj release

clean:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu clean
	$(MAKE) -f ${CURDIR}/../Makefile.pal clean
	$(MAKE) -f ${CURDIR}/../Makefile.ntscj clean
	rm -f *.bin *.elf

clean-release:
	$(MAKE) -f ${CURDIR}/../Makefile.ntscu clean-release
	$(MAKE) -f ${CURDIR}/../Makefile.pal clean-release
	$(MAKE) -f ${CURDIR}/../Makefile.ntscj clean-release
