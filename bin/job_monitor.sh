#!/bin/bash

# Only allow a single instance of this job monitor
for pid in $(pidof -x job_monitor.sh); do
    if [ $pid != $$ ]; then
        exit 0
    fi
done

sleep 10

# Monitor jobs indefinitely (or until all jobs have completed)
while true; do
    TIMEOUT=600

    JOBS="$(condor_status -submitters | grep filip | head -1)"

    RUNNING="$(echo $JOBS | awk '{print $3}')"
    IDLE="$(echo $JOBS | awk '{print $4}')"
    HELD="$(echo $JOBS | awk '{print $5}')"

    # exit with success if all jobs are completed
    if [ $(( $RUNNING + $IDLE + $HELD )) == 0 ]; then
        ssh -p22222 quizznor@localhost notify-send -a SSH "'HTCondor JOB_COMPLETE!' 'All jobs have finished'"
        exit 0
    fi

    # panic if any jobs are on hold
    if [ "$HELD" != "0" ]; then
        ssh -p22222 quizznor@localhost notify-send -a SSH "'HTCondor ON_HOLD alert!' 'There are $HELD jobs on hold'"
        TIMEOUT=60
    fi

    sleep $TIMEOUT
done


