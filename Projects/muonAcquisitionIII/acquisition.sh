#!/bin/bash

count=0
while [ true ]; do

	count=$((count + 1))
	echo "cleaning up and overwriting muonAcqStore.start..."
	IkLSend IkLsOS9  "rm -rf muonAcqStore.out; echo '2' > muonAcqStore.start" 943 944 949 954 
	sleep 5
	echo "executing muonAcquisition script..."
	IkLSend IkLsOS9 "./acq 999999999999 600 9999999999 70" 943 944 949 954 
	sleep 620
	echo "Downloading muonAcqStore.out..."
	IkLSend IkLsLogReq muonAcqStore.out 943 944 949 954 
	sleep 300
	mkdir -p data/$count
	mv /Raid/var/trash/* data/$count/
	
	touch data/$count/$(date +%s)
	echo "Sending T3"
	T3Send 943 944 949 954
	sleep 300
	echo "#${count} finished, starting over in 5s..."
	sleep 5

done
