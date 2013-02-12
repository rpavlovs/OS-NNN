
ROOT=$(HOME)/cs350-os161/root

SYMLINKS = testbin bin kernel
GENERATED = bigfile.out tailfile catfile hashfile badcallfile
TESTOUTPUT = out.A3 out.A3-option1 out.A3-option2
ASST = -ASST3

do: pgms install

all: pgms install runtests

runtests: 
	./run-batch-A3-all

pgms:
	cd vm-stack1; make depend; make; make install
	cd vm-stack2; make depend; make; make install
	cd vm-data1; make depend; make; make install
	cd vm-data2; make depend; make; make install
	cd vm-data3; make depend; make; make install
	cd vm-crash1; make depend; make; make install
	cd vm-crash2; make depend; make; make install
	cd vm-crash3; make depend; make; make install

dotar:
	cd ..; tar -czf a3-test-scripts.tar.gz `cat a3-test-scripts/tarlist`; cd a3-test-scripts

install:
	-ln -s $(ROOT)/testbin .
	-ln -s $(ROOT)/bin .
	-ln -s $(ROOT)/kernel$(ASST) kernel

clean:
	-/bin/rm $(SYMLINKS)
	-/bin/rm $(GENERATED)

vclean: clean
	-/bin/rm $(TESTOUTPUT)
	-/bin/rm DISK1.img DISK2.img
	-/bin/rm -rf newtests
	-/bin/rm tmp.got tmp.expected
	-/bin/rm out.A3*
	cd vm-stack1; make clean
	cd vm-stack2; make clean
	cd vm-data1; make clean
	cd vm-data2; make clean
	cd vm-data3; make clean
	cd vm-crash1; make clean
	cd vm-crash2; make clean
	cd vm-crash3; make clean
