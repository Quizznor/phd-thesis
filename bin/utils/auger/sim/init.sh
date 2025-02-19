#!/bin/bash

source $1
cd $2
cp Makefile.in Makefile
make
mv *.xml userAugerOffline $3/src
rm -rf Makefile *.o Make-depend