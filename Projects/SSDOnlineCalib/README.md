# SSD Online calibration

### The goal is to have a working online (i.e. station-level) calibration for the SSD. This will allow to use the SSD not only in slave-mode to the WCD, but also independent operation.

## Contents
1. Analysis/ - code related to constructing a new algorithm for SSD Online calibration
1. ExtractTraces/ - code to extract SSD/WCD traces from random-trace binary files (stored in /cr/tempdata01/filip/UubRandoms/)
2. OfflineCalib/ - code related to the histogram-based estimation of the MIP peak -- OUTDATED
3. OnlineCalib/ - code related to the rate-based estimation of the MIP peak -- OUTDATED
4. SSDTraceCharacteristics/ - scriplets to get a feel for SSD trace characteristics
5. Monitoring/ - code to read event data/monitoring data from the SD array