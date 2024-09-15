#!/bin/bash

string="# t wcd1 wcd2 wcd3 ssd"

tail -f /home/root/daq/error.log | while read -r line; do
# cat error.log | while read -r line; do

    # XbTrig T1 online threshold values
    if [[ "$line" =~ \[I\]Trigger2\([0-9]*\):\ T1\ Th ]]; then 
        t1Th=$(echo $line | awk {'print $4 " " $5 " " $6'} | sed "s/ / -1 /g")
        echo nan $t1Th -1 -1 -1
        echo nan $t1Th -1 -1 -1 >> calib.test
    fi
    
    # MuonFill timestamp
    if [[ "$line" =~ \[E\]MuonFill\([0-9]*\):\ M ]]; then
        echo $string
        echo $string >> calib.test
        string=$(echo $line | awk '{print $8}' | tr -d :)
    fi

    # MuonFill online threshold values
    if [[ "$line" =~ \[E\]MuonFill\([0-9]*\):\ (WC|SS)D ]]; then
        th=$(echo $line | awk '{print $NF}' | cut -c 7-)
        hz=$(echo $line | awk '{print $6}' | tr -d \(\)Hz)
        string="$string $th $hz"
    fi
done