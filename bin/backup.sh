#!/bin/bash

if [ -f "/cr/users/filip/bin/history.pickle" ]; then
    mv /cr/users/filip/bin/history.pickle /cr/data01/filip/.history/$(date '+%Y_%m_%_d_%H').pickle
fi
