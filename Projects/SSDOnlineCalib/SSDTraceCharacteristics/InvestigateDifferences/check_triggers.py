#!/bin/python3 

import sys, os
import numpy as np

q_peak = np.array([144, 149.7, 149.2])

def check_T1(trace : np.ndarray) -> int :
    
    calibrated_trace = trace / q_peak[:, None]

    for i in range(calibrated_trace.shape[1]):
        if calibrated_trace[0][i] > 1.75:
            if calibrated_trace[1][i] > 1.75:
                if calibrated_trace[2][i] > 1.75:
                    return 1
                else: continue
            else: continue
        else:continue
    else: return 0 

wcd_file=f"/cr/tempdata01/filip/UubRandoms/Nov2022/converted/SvenjaFiltered/randoms0009_WCD.dat"

traces = np.loadtxt(wcd_file)
traces = np.split(traces, len(traces) // 3)

t1_info = 0
for i, trace in enumerate(traces):
    is_t1 = check_T1(trace)
    if is_t1: print(i, end = " ")
    t1_info += check_T1(trace)

print(f"\nPython found {t1_info}")

# os.system(f"rm -rf {wcd_file}")