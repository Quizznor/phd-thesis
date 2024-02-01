import sys, os

sys.path.append('/cr/users/filip/bin')

from utils.binaries import *
from utils.plotting import *

import multiprocessing as mp
from scipy.stats import norm

thresholds = np.round(np.arange(1.2, 2.51, 0.05),2)

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
    this_result = f"{_id} {time} {mip:.2f} "
    for t in thresholds:
        this_result += f"{sum([_ > t * mip for _ in full_histogram]) / 61:.2f} "

    # quality control to plot histograms for 'abnormally'(?) high mip values
    # if mip > 60:
    #     q.put(-1)
    #     plt.figure()
    #     plt.plot(ADC, counts)
    #     plt.axvline(mip)
    #     plt.yscale('log')
    #     plt.savefig(f'/cr/users/filip/plots/{time}_{_id}.png')

    q.put(1)

    return this_result

# def writer(q):
#     with open('test_multi_processing.txt', 'w') as file:
#         while True:
#             message = q.get()
#             if message == 'kill':
#                 break
#             else:
#                 file.write(message + '\n')
#                 file.flush()

POOLSIZE = 100

infile = open('/cr/users/filip/Data/SDMonitHistos/all.txt', 'r')
iterator = iter(infile)
manager = mp.Manager()
q = manager.Queue()
pool = mp.Pool(mp.cpu_count() + 2)                             # use 16 cores?

outfile = open('/cr/tempdata01/filip/SSDCalib/BootstrapHistos/rate_spread_study.txt', 'w')
outfile.write(f"#id t mip {' '.join([str(t) + 'xmip' for t in thresholds])}\n")

print()
while True:
    try:
        
        jobs = []
        for _ in range(POOLSIZE):
            _id, time, *histo = next(iterator).split()

            job = pool.apply_async(worker, (histo, _id, time, q))
            jobs.append(job)

            # print(f'processing lines {n - POOLSIZE}-{n}. Last id: {_id}...', end='\r')
        
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