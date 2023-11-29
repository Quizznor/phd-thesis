#!/bin/python3 

import sys, os
import numpy as np

station = sys.argv[1]
date = sys.argv[2]

q_peak = {
    # 'Jaco' :                            np.array([189.4, 164.3, 158.6]),
    # 'Granada' :                         np.array([153.5, 160.6, 170.0]),
    # 'NuriaJrFiltered' :                 np.array([132.5, 135.2, 133.6]),
    # 'NuriaJrFilteredOnline' :           np.array([156.5, 163.7, 162.7]),
    # 'NuriaJrFilteredT1' :               np.array([172.0, 184.5, 171.3]),
    # 'PeruFilteredT1' :                  np.array([172.4, 139.9, 171.1]),
    # 'SvenjaFiltered':                   np.array([134.3, 144.1, 134.5]),
    # 'NadiaEarlyFilteredDownsampled' :   np.array([114.9, 124.4, 116.8]),
    # 'NadiaLateFilteredDownsampled' :    np.array([114.0, 125.3, 113.6]),
    # 'NadiaLateFilteredDownsampledT1' :  np.array([148.9, 162.2, 151.8]),
    # 'NadiaEarlyFilteredDownsampledT1' : np.array([148.9, 162.2, 151.8]),
    # 'NadiaLateOnline' :                 np.array([144.0, 149.7, 149.2]),
    # 'NadiaEarlyOnline' :                np.array([144.0, 149.7, 149.2]),

    'NuriaJr' :                         np.array([156.50, 163.65, 162.70]),
    'NuriaJrFiltered' :                 np.array([156.50, 163.65, 162.70]),
    'NuriaJrFilteredDownsampled' :      np.array([156.50, 163.65, 162.70]),
    'PeruFilteredDownsampled' :         np.array([150.30, 117.50, 153.35]),
    'NadiaEarlyFilteredDownsampled' :   np.array([148.90, 162.20, 151.75]),
    'NadiaLateFilteredDownsampled' :    np.array([148.90, 162.20, 151.75]),
}

def check_T1(trace : np.ndarray) -> int :

    # Maybe adjust thresholds instead of calibrating trace?
    # likely faster and more reflective of actual algorithm 

    for i in range(trace.shape[1]):
        if trace[0][i] > threshold[0]:
            if trace[1][i] > threshold[1]:
                if trace[2][i] > threshold[2]:
                    return 1
                else: continue
            else: continue
        else:continue
    else: return 0 

os.system(f"mkdir -p /cr/tempdata01/filip/SSDCalib/UUBCrosscheck/{date}/{station}")
wcd_file=f"/cr/tempdata01/filip/UubRandoms/{date}/converted/{station}/randoms{int(sys.argv[3]):04d}_WCD.dat"

traces = np.loadtxt(wcd_file)
traces = np.split(traces, len(traces) // 3)

threshold = q_peak[station]
t1_info = []

for i, trace in enumerate(traces):
    t1_info.append(check_T1(trace))

np.savetxt(wcd_file.replace(f"UubRandoms/{date}/converted", f"SSDCalib/UUBCrosscheck/{date}"), np.array(t1_info), fmt='%i')
