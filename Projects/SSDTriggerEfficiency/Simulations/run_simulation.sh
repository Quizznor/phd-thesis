#!/bin/bash

# echo $@
source /cr/data01/filip/offline/install/ds_forced_station_triggers/set_offline_env.sh

# cd ../SdSimulationReconstructionUpgrade
./userAugerOffline --bootstrap $1
rm -rf SdSimAndRecOffline.root
# cd ../ADSTReader
./AdstReader $2

rm -rf $@
