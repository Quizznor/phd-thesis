#!/bin/bash

echo $@
source $OFFLINE

cd ../SdSimulationReconstruction
./userAugerOffline --bootstrap $1
cd ../ADSTReader
./AdstReader $2

# rm -rf ../Offline/HybridRec.dat #$@
