import sys, os

sys.path.append('/cr/users/filip/bin')

from utils.binaries import *
from utils.plotting import *

import multiprocessing as mp
from scipy.stats import norm

calc_rate = lambda x, y : sum([_ > y for _ in x]) / 61
thresholds, mean_rates = np.loadtxt('/cr/users/filip/Data/SDMonitHistos/mean_rates.txt', unpack=True)

# see Station.h in Framework/SDetector
ADC_bin_edges = np.array([2*k for k in range(100)] + [200 + 8*k for k in range(51)])
ADC = 0.5 * (ADC_bin_edges[1:] + ADC_bin_edges[:-1])
ADC_width = np.diff(ADC_bin_edges)

def make_histo(counts, skip_to_threshold = 5):
    
    histo = []

    for i, count in enumerate(counts[skip_to_threshold:]):
        idx = i + skip_to_threshold
        width = ADC_width[idx]
        wiggle = np.random.uniform(-0.5*width, 0.5*width, int(count))
        array = np.array([ADC[idx] for _ in range(int(count))])
        histo += [w + a for w, a in zip(wiggle, array)]

    return histo

def worker(counts, _id, time, q):

    counts = [int(_) for _ in counts]
    full_histogram = make_histo(counts)
    mip, _ = norm.fit(full_histogram)
    this_result = f"{_id} {time} {mip:.4f} "

    full_histogram.sort()
    for threshold, target_rate in zip(thresholds, mean_rates):
        it, rate = iter(full_histogram[::-1]), 0
        min_diff = np.inf

        while True:
            rate += 1/61
            this_diff = np.abs(target_rate - rate)
            rate_based_mip = next(it)

            if min_diff < this_diff: break
            
            min_diff = this_diff

        this_result += f"{rate_based_mip / threshold:.4f} "

    print(this_result)

    q.put(1)

    return this_result

POOLSIZE = 100

infile = open('/cr/users/filip/Data/SDMonitHistos/all.txt', 'r')
iterator = iter(infile)
manager = mp.Manager()
q = manager.Queue()
pool = mp.Pool(mp.cpu_count() + 2)                             # use 16 cores?

outfile = open('/cr/tempdata01/filip/SSDCalib/BootstrapHistos/mean_rate_deviation_study.txt', 'w')
outfile.write(f"#id t mip {' '.join([str(t) + 'xmip' for t in thresholds])}\n")

sys.exit('have you updated mean_rates from second_look.ipynb?')

print()
while True:
    try:
        
        jobs = []
        for _ in range(POOLSIZE):
            _id, time, *histo = next(iterator).split()

            job = pool.apply_async(worker, (histo, _id, time, q))
            jobs.append(job)

        for job in jobs:
            outfile.write(job.get() + '\n')

        n_success = 0
        while not q.empty():
            n_success += q.get()

        assert n_success == POOLSIZE, 'Some job could not be finished! =('



    except StopIteration: break

infile.close()
outfile.close()
pool.close()
pool.join()