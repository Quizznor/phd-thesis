#!/bin/bash

BASE=/cr/data01/filip/Data/muonAcquisitionIII/data/*

for dir in $BASE; do
    for file in $dir/*; do
        record/r $file > $file.out
    done 
done

