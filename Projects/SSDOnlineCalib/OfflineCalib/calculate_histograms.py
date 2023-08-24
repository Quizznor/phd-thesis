#!/bin/python3

import sys
import numpy as np

def single_bin_trigger(trace, threshold : int, multiplicity : int = 1) -> int :

    if isinstance(trace, np.ndarray):
        return sum(trace >= threshold) >= multiplicity

    elif isinstance(trace, list):
        pmt1, pmt2, pmt3 = trace
        for b in range(len(pmt1)):
            t1 = pmt1[b] >= threshold
            t2 = pmt2[b] >= threshold
            t3 = pmt3[b] >= threshold

            if sum((t1, t2, t3)) >= multiplicity:
                return True
        else:
            return False

def threshold_trigger(trace : list, threshold : float = 1.75) -> int :
    # q_peak comes from analysis done during my Masters'
    q_peak = np.array([180.23, 182.52, 169.56]) * (1 - 11.59/100)

    pmt1, pmt2, pmt3 = trace
    pmt1 = pmt1 / q_peak[0]
    pmt2 = pmt2 / q_peak[1]
    pmt3 = pmt3 / q_peak[2]

    # hierarchy doesn't (shouldn't?) matter
    for i in range(len(trace[0])):
        if pmt1[i] > threshold:
            if pmt2[i] > threshold:
                if pmt3[i] > threshold:
                    return True
                else: continue
            else: continue
        else: continue
    
    return False

file=f"/cr/tempdata01/filip/SSDCalib/RadioCut/randoms{int(sys.argv[1]) + 1:04d}"

# Load data and split into appropriate structure
WCD_data = np.loadtxt(f"{file}_WCD.dat", dtype=int)
WCD_data = np.split(WCD_data, len(WCD_data) // 3)

SSD_data = np.loadtxt(f"{file}_SSD.dat", dtype=int)

thresholds = range(1,351)
SSD_rates_T1 = np.zeros_like(thresholds)
SSD_rates = np.zeros_like(thresholds)

# Subtract baseline and perform rate calculation
for step, (SSD, WCD) in enumerate(zip(SSD_data, WCD_data)):

    #print(f"{step}/5000", end = "\r")
    SSD = SSD[1:] - SSD[0]
    WCD = [pmt[1:] - pmt[0] for pmt in WCD]

    is_T1 = threshold_trigger(WCD)

    for t in thresholds:
        is_SB = single_bin_trigger(SSD, t)
        SSD_rates_T1[t-1] += (is_SB and is_T1)
        SSD_rates[t-1] += is_SB

np.savetxt(f"/cr/tempdata01/filip/SSDCalib/Rates/with_T1/randoms_{int(sys.argv[1]) +1:04d}.dat", SSD_rates_T1)
np.savetxt(f"/cr/tempdata01/filip/SSDCalib/Rates/without_T1/randoms_{int(sys.argv[1]) +1:04d}.dat", SSD_rates)
