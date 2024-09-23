#!/bin/bash

echo $@
source /cr/data01/filip/offline/install/ds_forced_station_triggers/set_offline_env.sh

cd /cr/users/filip/Projects/SSDTriggerEfficiency/SdSimulationReconstructionUpgrade
./userAugerOffline --bootstrap $1
rm -rf SdSimAndRecOffline.root
cd /cr/users/filip/Projects/SSDTriggerEfficiency/ADSTReader
./AdstReader $2

# rm -rf $@
