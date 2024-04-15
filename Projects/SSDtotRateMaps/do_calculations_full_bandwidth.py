#!/bin/python3

import sys
sys.path.append('/cr/users/filip/bin/')

from utils.Auger.SD.UubRandoms import *
from itertools import product
from utils.binaries import *

def time_over_threshold(trace : np.ndarray, threshold : float = 0.2, multiplicity : int = 12) -> bool :

    first_120_bins = trace[:120]
    pmt_running_sum = (first_120_bins >= threshold).sum()

    for i in range(120, trace.shape[0] + 1):
        if pmt_running_sum > multiplicity: return True                      # check multiplicity for each window
        # if i == traces.shape[0]: return False                               # we've reached the end of the trace

        new_over_threshold = np.array(trace[i] > threshold, dtype=int)
        old_over_threshold = np.array(trace[i - 120] > threshold, dtype=int)
        pmt_running_sum += new_over_threshold - old_over_threshold

    return False

def time_over_threshold_deconvoluted(trace : np.ndarray, threshold : float = 0.2, multiplicity : int = 12) -> bool : 

    # for information on this see GAP note 2018-01
    dt      = 25                                                                # UB bin width
    tau     = 67                                                                # decay constant
    decay   = np.exp(-dt/tau)                                                   # decay term
    deconvoluted_trace = []

    deconvoluted_pmt = [(trace[i] - trace[i-1] * decay)/(1 - decay) for i in range(1,len(trace))]
    deconvoluted_trace.append(deconvoluted_pmt)

    return time_over_threshold(np.array(deconvoluted_trace), threshold, multiplicity)

STATION = sys.argv[1]

# 300 scanning points
multiplicities = range(5, 45)
threshold = np.linspace(0.05, 3.00, 295)
params = list(product(multiplicities, threshold))

print(len(params))

m, t = params[int(sys.argv[2])]
try:
    already_calculated = np.loadtxt(f'/cr/users/filip/Data/SSDtotRateMap/{STATION}_SSD.txt', usecols=[0, 1])
    for _m, _t in already_calculated:
        if _m == m and _t == t: sys.exit(f"calculation already exists for {m, t}")
except FileNotFoundError:
    pass

tot_sum, totd_sum = 0, 0
total_time = 0

for File in tools.ProgressBar(UubRandom(f'{STATION}', 'ssd'), newline=False):
    traces = File['trace']
    mip_peaks = File['mip_peak']

    for trace, mip_peak in zip(traces, mip_peaks):
        calibrated = trace / mip_peak[:, np.newaxis]

        tot_sum += time_over_threshold(calibrated, t, m)
        totd_sum += time_over_threshold_deconvoluted(calibrated, t, m)
        total_time += 2048 * 8.33e-9

with open(f'/cr/users/filip/Data/SSDtotRateMap/{STATION}_SSD.txt', 'a+') as file:
    file.write(f"{m} {t} {tot_sum} {totd_sum} {total_time}\n")