#!/bin/bash

echo $@
source $OFFLINE

cd ../Offline
./userAugerOffline --bootstrap $1
cd ../ADSTReader
./AdstReader $2

rm -rf ../Offline/HybridRec.dat $1 #$2
