#1/bin/bash

while ssh lyon 'test -e .irods_copy_in_progress'; do
    while ssh lyon 'test -e XzScanner/.copying'; do sleep 3; done

    scp -r pafilip@cca.in2p3.fr:XyScanner/temp/* /cr/data02/AugerPrime/XY/2023oct_calA/ 2> /dev/null
    ssh lyon "rm -rf XyScanner/temp/* && rm -rf XyScanner/.copying"
done
