#!/bin/python3 

import sys, os
import numpy as np

station = sys.argv[1]
date = sys.argv[2]

q_peak = {
    'Jaco' : np.array([189.4, 164.3, 158.6]),
    'Granada' : np.array([153.5, 160.6, 170]),
    'NuriaJr' : np.array([169.3, 176.9, 169.5]),
    'Svenja' : np.array([172.4, 187, 168.8]),
    'SvenjaDownsampled': np.array([154.4, 165.9, 150.2]),
    
}

def check_T1(trace : np.ndarray) -> int :
    
    calibrated_trace = (trace[:,1:] - trace[:,0][:, None]) / q_peak[station][:, None]

    for i in range(calibrated_trace.shape[1]):
        if calibrated_trace[0][i] >= 1.75:
            if calibrated_trace[1][i] >= 1.75:
                if calibrated_trace[2][i] >= 1.75:
                    return 1
                else: continue
            else: continue
        else:continue
    else: return 0 

os.system(f"mkdir -p /cr/tempdata01/filip/SSDCalib/WCDT1Calib/{date}/{station}")
wcd_file=f"/cr/tempdata01/filip/UubRandoms/{date}/converted/{station}/randoms{int(sys.argv[3]) + 1:04d}_WCD.dat"

traces = np.loadtxt(wcd_file)
traces = np.split(traces, len(traces) // 3)

t1_info = []
for trace in traces:
    t1_info.append(check_T1(trace))

print(wcd_file.replace(f"UubRandoms/{date}/converted", f"SSDCalib/WCDT1Calib/{date}"))
np.savetxt(wcd_file.replace(f"UubRandoms/{date}/converted", f"SSDCalib/WCDT1Calib/{date}"), np.array(t1_info), fmt='%i')

# os.system(f"rm -rf {wcd_file}")