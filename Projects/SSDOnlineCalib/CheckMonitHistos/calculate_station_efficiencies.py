#!/usr/bin/python

import sys, os
sys.path.append('/cr/users/filip/bin/')

from utils.binaries import *
from utils.plotting import *

date, station = sys.argv[1] + '/', sys.argv[2] + '/'
root_wcd = '/cr/tempdata01/filip/SSDCalib/UUBCrosscheck/'
root_ssd = f'/cr/tempdata01/filip/UubRandoms/{date}/converted/'

# # this comes from look_into_randoms.ipynb
# MIP = {
#     'Nadia':        49.5,
#     'NuriaJr':      53.7,}

n_present_files = len(os.listdir(root_ssd + station)) // 2
ssd_and_t1, ssd_only = [], []

q_peak = {
    'NuriaJrFilteredDownsampled/' :      np.array([156.50, 163.65, 162.70]),
    'PeruFilteredDownsampled/' :         np.array([150.30, 117.50, 153.35]),
    'NadiaEarlyFilteredDownsampled/' :   np.array([148.90, 162.20, 151.75]),
    'NadiaLateFilteredDownsampled/' :    np.array([148.90, 162.20, 151.75]),
}

def get_latch_bin(wcd_trace):
    for i, (b1, b2, b3) in enumerate(zip(*wcd_trace)):

        if b1 / q_peak[station][0] < 1.75: continue
        if b2 / q_peak[station][1] < 1.75: continue
        if b3 / q_peak[station][2] < 1.75: continue
        
        return i * 3
    else: return -1

for i_file in range(n_present_files):

    tools.progress_bar(i_file, n_present_files)

    try:
        wcd_data = np.loadtxt(root_ssd + station + f'randoms{i_file:04}_WCD.dat')
        wcd_data = np.split(wcd_data, len(wcd_data) // 3)
        ssd_data = np.loadtxt(root_ssd + station + f'randoms{i_file:04}_SSD.dat')
        t1_data  = np.loadtxt(root_wcd + date + station + f'randoms{i_file:04}_WCD.dat')
    except (FileNotFoundError, ValueError):
        continue

    for i_trace, (ssd, t1) in enumerate(zip(ssd_data, t1_data)):

        ssd_trace = ssd[1:] - ssd[0]
        argmax = np.argmax(ssd_trace)

        if not t1: ssd_only.append(ssd_trace[argmax])
        else:
            wcd_t1_bin = get_latch_bin(wcd_data[i_trace])
            if argmax - 20 < wcd_t1_bin <= argmax + 49:
                ssd_and_t1.append(ssd_trace[argmax])
            else:
                ssd_only.append(ssd_trace[argmax])

np.savetxt(f'/cr/users/filip/Data/StationEfficiencies/{station[:-1]}_ssd_only.txt', np.array(ssd_only))
np.savetxt(f'/cr/users/filip/Data/StationEfficiencies/{station[:-1]}_ssd_and_t1.txt', np.array(ssd_and_t1))
