#!/bin/bash

echo $@
source $OFFLINE

cd ../SdSimulation
./userAugerOffline --bootstrap $1
cd ../ADSTReader
./AdstReader $2

rm -rf ../Offline/HybridRec.dat #$@
