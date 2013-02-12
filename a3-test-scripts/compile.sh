#!/bin/bash
#Compile and install os161 kernel

cd ~/cs350-os161/os161-1.11
./configure --ostree=$HOME/cs350-os161/root --toolprefix=cs350-
cd ~/cs350-os161/os161-1.11/kern/conf
./config ASST$1
cd ~/cs350-os161/os161-1.11/kern/compile/ASST$1
make depend
make
make install

