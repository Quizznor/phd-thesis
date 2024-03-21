#!.venv/bin/python

from utils.Auger.SD.UubRandoms import *
from itertools import product
from utils.binaries import *
import sys

def time_over_threshold(traces : np.ndarray, threshold : float = 0.2, multiplicity : int = 12) -> bool :

    windows = np.lib.stride_tricks.sliding_window_view(traces, (3, 120))[0]

    for pmt1, pmt2, pmt3 in windows:

        pmt1_active = (pmt1 > threshold).sum() >= multiplicity
        pmt2_active = (pmt2 > threshold).sum() >= multiplicity
        pmt3_active = (pmt3 > threshold).sum() >= multiplicity

        if sum([pmt1_active, pmt2_active, pmt3_active]) > 1:
            return True
    else: return False


STATION = 'NuriaJr'

multiplicities = range(5, 20)
threshold = np.linspace(0.01, .25, 25)
params = list(product(multiplicities, threshold))

m, t = params[int(sys.argv[1])]
tot_sum, totd_sum = 0, 0

for File in tools.ProgressBar(UubRandom('NuriaJr')):
    traces = File['traces']
    vem_peak = File['vem_peak']
    calibrated_traces = traces / vem_peak[:, :, np.newaxis]
    for trace in calibrated_traces:
        tot_sum += time_over_threshold(trace, t, m)
        totd_sum += time_over_threshold_deconvoluted(trace, t, m)