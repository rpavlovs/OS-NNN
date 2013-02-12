# Makefile for add

PROG=vm-stack2
SRCS=$(PROG).c
BINDIR=/../a3-test-scripts/newtests

include ../../defs.mk
include ../../mk/prog.mk

all: $(PROG)

EXTRAS = readelf.out nm.out objdump.out $(PROG).S 

extras: $(EXTRAS)

readelf.out:	$(PROG).c $(PROG)
	cs350-readelf -a $(PROG) > readelf.out

objdump.out:	$(PROG).c $(PROG)
	cs350-objdump -s $(PROG) > objdump.out

$(PROG).S:	$(PROG).c $(PROG)
	cs350-objdump -d $(PROG) > $(PROG).S

nm.out:	$(PROG).c $(PROG)
	cs350-nm -n $(PROG) > nm.out

vclean: clean
	rm $(EXTRAS)

