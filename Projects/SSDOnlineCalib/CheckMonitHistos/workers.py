import sys, os

sys.path.append('/cr/users/filip/bin')

from utils.binaries import *
# from utils.plotting import *

import multiprocessing as mp
from scipy.optimize import curve_fit

import warnings
warnings.simplefilter("ignore", RuntimeWarning)

# for whatever reason there's a bug which forces you to define worker functions 
# in multiprocessing pools OUTSIDE the running script, and then import them ...

# read more here: https://github.com/python/cpython/issues/69240

def parabola(x, a, b, c):
    return a * x**2 + b*x + c

def make_histo(counts, skip_to_threshold = 12):
    
    # see Station.h in Framework/SDetector
    ADC_bin_edges = np.array([2*k for k in range(100)] + [200 + 8*k for k in range(51)])
    ADC = 0.5 * (ADC_bin_edges[1:] + ADC_bin_edges[:-1])
    ADC_width = np.diff(ADC_bin_edges)    
    histo = []

    for i, count in enumerate(counts[skip_to_threshold:]):
        idx = i + skip_to_threshold
        width = ADC_width[idx]
        wiggle = np.random.uniform(-0.5*width, 0.5*width, int(count))
        array = np.array([ADC[idx] for _ in range(int(count))])
        histo += [w + a for w, a in zip(wiggle, array)]

    return histo

def make_histo_fit(histogram, _id, time):
    binned, edges = np.histogram(histogram, bins=1000)
    position_of_peak = np.argmax(binned)
    start, stop = max(0, position_of_peak - 20), position_of_peak + 50
    x_fit = 0.5 * (edges[1:] + edges[:-1])

    try:
        (a, b, _), pp = curve_fit(parabola, x_fit[start:stop], np.log(binned)[start:stop], 
                            bounds=[[-np.inf, 0, 0],[0, np.inf, np.inf]],
                            p0=[-1000, 1, 1e3],
                            check_finite=True)
    
    except ValueError:
        a, b, _ = np.nan, np.nan, np.nan

    # if -b/(2*a) is np.nan:
    #     plt.figure()
    #     plt.plot(x_fit, np.exp(parabola(x_fit, a, b, _)), label='lstsq. fit')
    #     plt.plot(x_fit, binned, label='SSD histo')
    #     plt.axvline(x_fit[start], ls='--')
    #     plt.axvline(x_fit[stop], ls='--')
    #     plt.yscale('log')
    #     plt.legend()
    #     plt.savefig(f'/cr/users/filip/plots/failed_{_id}_{time}.png')

    return -b/(2*a) if 10 < -b/(2*a) < 250 else np.nan

def calc_rate_worker(counts, _id, time, q):

    thresholds = np.round(np.arange(1.0, 5.01, 0.05),2)
    counts = [int(_) for _ in counts]
    full_histogram = make_histo(counts)
    mip = make_histo_fit(full_histogram, _id, time)

    this_result = f"{_id} {time} {mip:.2f} "

    if mip is np.nan:
        this_result += len(thresholds) * f"{np.nan} "
        this_result = this_result[:-1]
    else:
        for t in thresholds:
            this_result += f"{np.sum([_ > t * mip for _ in full_histogram]) / 61:.2f} "
    q.put(1)

    return this_result

def calc_threshold_worker(counts, _id, time, q):

    thresholds, mean_rates = np.loadtxt('/cr/users/filip/Data/SDMonitHistos/mean_rates.txt', unpack=True)
    counts = [int(_) for _ in counts]
    full_histogram = make_histo(counts)
    mip = make_histo_fit(full_histogram, _id, time)

    this_result = f"{_id} {time} {mip:.4f} "
    full_histogram.sort()
    for threshold, target_rate in zip(thresholds, mean_rates):
        it, rate = iter(full_histogram[::-1]), 0
        min_diff = np.inf

        while True:
            rate += 1/61
            this_diff = np.abs(target_rate - rate)

            try:
                rate_based_mip = next(it)
            except StopIteration:
                rate_based_mip = np.nan
                break

            if min_diff < this_diff: break
            
            min_diff = this_diff

        this_result += f"{rate_based_mip / threshold:.4f} "

    q.put(1)

    return this_result

