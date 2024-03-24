#!/bin/python3

import sys
sys.path.append('/cr/users/filip/bin/')

from utils.Auger.SD.UubRandoms import *
from itertools import product
from utils.binaries import *

def time_over_threshold(traces : np.ndarray, threshold : float = 0.2, multiplicity : int = 12) -> bool :

    windows = np.lib.stride_tricks.sliding_window_view(traces, (3, 120))[0]

    for pmt1, pmt2, pmt3 in windows:

        pmt1_active = (pmt1 > threshold).sum() >= multiplicity
        pmt2_active = (pmt2 > threshold).sum() >= multiplicity
        pmt3_active = (pmt3 > threshold).sum() >= multiplicity

        if sum([pmt1_active, pmt2_active, pmt3_active]) > 1:
            return True
    else: return False

def time_over_threshold_deconvoluted(traces : np.ndarray, threshold : float = 0.2, multiplicity : int = 12) -> bool : 

    # for information on this see GAP note 2018-01
    dt      = 25                                                                # UB bin width
    tau     = 67                                                                # decay constant
    decay   = np.exp(-dt/tau)                                                   # decay term
    deconvoluted_trace = []

    for pmt in traces:
        deconvoluted_pmt = [(pmt[i] - pmt[i-1] * decay)/(1 - decay) for i in range(1,len(pmt))]
        deconvoluted_trace.append(deconvoluted_pmt)

    return time_over_threshold(np.array(deconvoluted_trace), threshold, multiplicity)

STATION = 'Constanza'

# 300 scanning points
multiplicities = range(2, 14)
threshold = np.linspace(0.01, .25, 25)
params = list(product(multiplicities, threshold))

m, t = params[int(sys.argv[1])]
tot_sum, totd_sum = 0, 0
total_time = 0

for File in tools.ProgressBar(UubRandom('NuriaJr', 'wcd'), newline=False):
    traces = File['traces']
    vem_peak = File['vem_peak']

    for trace, vem in zip(traces, vem_peak):
        filter_and_downsampled = filter_and_downsample(*trace)
        calibrated = filter_and_downsampled / vem[:, np.newaxis]

        tot_sum += time_over_threshold(calibrated, t, m)
        totd_sum += time_over_threshold_deconvoluted(calibrated, t, m)
        total_time += 2048 * 8.33e-9

with open(f'/cr/users/filip/Data/totRateMap/{STATION}.txt', 'a+') as file:
    file.write(f"{m} {t} {tot_sum} {totd_sum} {total_time}\n")