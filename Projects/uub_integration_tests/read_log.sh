#!/bin/bash

string="# t wcd1 wcd2 wcd3 ssd"

tail -f $1 | while read -r line; do

    # XbTrig T1 online threshold values
    if [[ "$line" =~ \[I\]Trigger2\([0-9]*\):\ T1\ Th ]]; then 
        t1Th=$(echo $line | awk '{$1=$2=$3=""; print $0}')
        echo nan $t1Th -1 | tee calib.test
    fi
    
    # MuonFill timestamp
    if [[ "$line" =~ \[E\]MuonFill\([0-9]*\):\ M ]]; then
        echo $string | tee calib.test
        string=$(echo $line | awk '{print $8}')
    fi

    # MuonFill online threshold values
    if [[ "$line" =~ \[E\]MuonFill\([0-9]*\):\ (WC|SS)D ]]; then
        th=$(echo $line | awk '{print $NF}' | cut -c 7-)
        string="$string $th"
    fi
done