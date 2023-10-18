#!/usr/bin/python3

import sys, os
import numpy as np

def apply_downsampling(pmt : np.ndarray) -> np.ndarray :

        n_bins_uub      = (len(pmt) // 3) * 3               # original trace length
        n_bins_ub       = n_bins_uub // 3                   # downsampled trace length
        sampled_trace   = np.zeros(n_bins_ub)               # downsampled trace container

        # ensure downsampling works as intended
        # cuts away (at most) the last two bins
        if len(pmt) % 3 != 0: pmt = pmt[0 : -(len(pmt) % 3)]

        # see Framework/SDetector/UUBDownsampleFilter.h in Offline main branch for more information
        kFirCoefficients = [ 5, 0, 12, 22, 0, -61, -96, 0, 256, 551, 681, 551, 256, 0, -96, -61, 0, 22, 12, 0, 5 ]
        buffer_length = int(0.5 * len(kFirCoefficients))
        kFirNormalizationBitShift = 11

        temp = np.zeros(n_bins_uub + len(kFirCoefficients))

        temp[0 : buffer_length] = pmt[:: -1][-buffer_length - 1 : -1]
        temp[-buffer_length - 1: -1] = pmt[:: -1][0 : buffer_length]
        temp[buffer_length : -buffer_length - 1] = pmt

        # perform downsampling
        for j, coeff in enumerate(kFirCoefficients):
            sampled_trace += [temp[k + j] * coeff for k in range(0, n_bins_uub, 3)]

        # clipping and bitshifting
        sampled_trace = [int(adc) >> kFirNormalizationBitShift for adc in sampled_trace]

        # Simulate saturation of PMTs at 4095 ADC counts ~ 19 VEM <- same for HG/LG? I doubt it
        return np.array(sampled_trace)

file_to_convert = f"/cr/tempdata01/filip/iRODS/UubRandoms/converted/{sys.argv[1]}/randoms{int(sys.argv[2]):04d}_WCD.dat"
data = np.loadtxt(file_to_convert)
downsampled_data = []

os.system(f"mkdir /cr/tempdata01/filip/iRODS/UubRandoms/converted/{sys.argv[1]}Downsampled")
with open(file_to_convert.replace(f"{sys.argv[1]}", f"{sys.argv[1]}Downsampled"), "w") as f:
    for trace in data:

        new_data = apply_downsampling(trace[1:])
        f.write(f"{new_data[0]} {' '.join([str(i) for i in new_data])}\n")
