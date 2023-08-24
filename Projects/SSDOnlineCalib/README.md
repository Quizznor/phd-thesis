# SSD Online calibration

### The goal is to have a working online (i.e. station-level) calibration for the SSD. This will allow to use the SSD not only in slave-mode to the WCD, but also independent operation.

## Contents
1. ExtractTraces/ - code to extract SSD/WCD traces from random-trace binary files (stored in /cr/tempdata01/filip/iRODS/)
2. OfflineCalib/ - code related to the histogram-based estimation of the MIP peak
3. OnlineCalib/ - code related to the rate-based estimation of the MIP peak
4. SSDTraceCharacteristics/ - scriplets to get a feel for SSD trace characteristics