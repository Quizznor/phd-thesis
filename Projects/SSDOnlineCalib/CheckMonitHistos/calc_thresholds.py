import sys, os

sys.path.append('/cr/users/filip/bin')

from utils.binaries import *
from utils.plotting import *
from workers import calc_threshold_worker

import multiprocessing as mp

POOLSIZE = 100
thresholds = np.round(np.arange(1.0, 2.51, 0.05),2)

infile = open('/cr/users/filip/Data/SDMonitHistos/all.txt', 'r')
iterator = iter(infile)
manager = mp.Manager()
q = manager.Queue()
pool = mp.Pool(mp.cpu_count() + 2)

outfile = open('/cr/tempdata01/filip/SSDCalib/BootstrapHistos/mean_rate_deviation_study.txt', 'w')
outfile.write(f"#id t mip {' '.join([str(t) + 'xmip' for t in thresholds])}\n")

print()
n_completed = 0
while True:
    try:
        jobs = []
        for _ in range(POOLSIZE):
            _id, time, *histo = next(iterator).split()

            job = pool.apply_async(calc_threshold_worker, (histo, _id, time, q))
            jobs.append(job)

        for job in jobs:
            outfile.write(job.get() + '\n')

        n_success = 0
        while not q.empty():
            n_success += q.get()

        assert n_success == POOLSIZE, 'Some job could not be finished! =('

        n_completed += n_success
        print(f"{n_completed: 6}/139366 : {n_completed/139366 * 100:.2f}%", end='\r')

    except StopIteration: break

infile.close()
outfile.close()
pool.close()
pool.join()