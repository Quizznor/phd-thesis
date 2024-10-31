#!/bin/bash

echo > calib.test
string="# t wcd1 wcd2 wcd3 ssd"

tail -f /home/root/daq/error.log | while read -r line; do
# cat error.log | while read -r line; do

#    # XbTrig T1 online threshold values
#    if [[ "$line" =~ \[I\]Trigger2\([0-9]*\):\ T1\ Th ]]; then 
#        t1Th=$(echo $line | awk {'print $4 " " $5 " " $6'} | sed "s/ / -1 /g")
#        echo nan $t1Th -1 -1 -1
#        echo nan $t1Th -1 -1 -1 >> calib.test
#    fi

    # MuonFill timestamp
    if [[ "$line" =~ ^\[I\]MuonFill\([0-9]*\):\ time= ]]; then
	echo $string >> calib.test
	string=$(echo $line | awk '{print $NF}')
	echo $line
    fi	 

    # MuonFill online threshold values
    if [[ "$line" =~ ^\[I\]MuonFill\([0-9]*\):\ old ]]; then
	echo $line
	info=$(echo $line | awk '{for (i=6; i<=NF; i++) print $i}' | tr -d '()')
	string="$string $info"
    fi
done