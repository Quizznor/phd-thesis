#!/bin/bash

echo $@
source $OFFLINE

cd ../Offline
./userAugerOffline --bootstrap $1

# BOOTSTRAP_TRG="/cr/work/filip/Simulations/Bootstraps/bootstrap_1337.xml"
# BOOTSTRAP_SRC="./bootstrap.xml.in"

# # Prepare bootstrap
# rm -rf $BOOTSTRAP_TRG


# INPUT='NR==59 {$0=FILE} '
# PATTERN='NR==62 {$0=PATTERN1} NR==66 {$0=PATTERN2} '
# OUTPUT='NR==81 {$0="/cr/tempdata01/filip/QGSJET-II/LTP/"ENERGY"/"NAME"_"DSEED".root"} '
# # OUTPUT='NR==95 {$0="/cr/users/filip/Simulation/TestOutput/"NAME".root"} '
# SEED='NR==100 {$0=DSEED} NR==103 {$0=PSEED} { print }'
# AWK_CMD=$INPUT$PATTERN$OUTPUT$SEED

# # # echo $3/$4.root

# # if [ ! -f $3$4_$6.root ]
# # then
# #     # Prepare bootstrap
#     awk -v FILE=$2_$1 -v PATTERN1="$7" -v PATTERN2="$8" -v NAME=$4 -v ENERGY=$5 -v DSEED=$6 -v PSEED="00000$(( $6 + 1 ))" "$AWK_CMD" $BOOTSTRAP_SRC > $BOOTSTRAP_TRG

#     # Run Simulation
#     /cr/users/filip/Simulation/AugerOffline/userAugerOffline --bootstrap $BOOTSTRAP

#     # Delete bootstrap
#     rm -rf $BOOTSTRAP
# fi

# # New ADST Component extractor! =)
# /cr/users/filip/Trigger/OfflineComparison/check_ADST/AdstReader $3$4_$6.root

# # Delete root file
# # rm -rf $3$4_$6.root

# ???
rm -rf /cr/users/filip/Projects/SSDTriggerEfficiency/Offline/HybridRec.dat