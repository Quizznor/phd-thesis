#!/bin/bash

if [ "$@" == "-all" ]; then
    killall job_monitor.sh >> /dev/null

condor_rm $@