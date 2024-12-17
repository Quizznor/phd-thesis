#!/usr/bin/python

import sys, os

sys.path.append("/cr/users/filip/bin/")

from utils.binaries import *
from utils.plotting import *

date, station = sys.argv[1] + "/", sys.argv[2] + "/"
root_wcd = "/cr/tempdata01/filip/SSDCalib/UUBCrosscheck/"
root_ssd = f"/cr/tempdata01/filip/UubRandoms/{date}/converted/"

q_peak = {
    "NuriaJrFilteredDownsampled/": np.array([156.50, 163.65, 162.70]),
    "PeruFilteredDownsampled/": np.array([150.30, 117.50, 153.35]),
    "NadiaEarlyFilteredDownsampled/": np.array([148.90, 162.20, 151.75]),
    "NadiaLateFilteredDownsampled/": np.array([148.90, 162.20, 151.75]),
}


def get_latch_bin(wcd_trace):
    for bin, (b1, b2, b3) in enumerate(zip(*wcd_trace)):

        if b1 / q_peak[station][0] < 1.75:
            continue
        if b2 / q_peak[station][1] < 1.75:
            continue
        if b3 / q_peak[station][2] < 1.75:
            continue

        return bin * 3
    else:
        return -1


print()
ssd_randoms_histogram = []
n_present_files, n_analyzed_files = len(os.listdir(root_ssd + station)) // 2, 0
for i_file in range(n_present_files):

    tools.progress_bar(i_file, n_present_files)

    try:
        wcd_data = np.loadtxt(root_ssd + station + f"randoms{i_file:04}_WCD.dat")
        wcd_data = np.split(wcd_data, len(wcd_data) // 3)
        ssd_data = np.loadtxt(root_ssd + station + f"randoms{i_file:04}_SSD.dat")
        n_analyzed_files += 1
    except (FileNotFoundError, ValueError):
        continue

    for i_trace, (ssd, wcd) in enumerate(zip(ssd_data, wcd_data)):

        if (latch_bin := get_latch_bin(wcd)) == -1:
            continue

        ssd_trace = ssd[1:] - ssd[0]
        ssd_trace_window = ssd_trace[
            np.max([latch_bin - 20, 0]) : np.min([latch_bin + 49, 2047])
        ]
        if (ssd_max := int(np.max(ssd_trace_window))) < 3 * np.std(ssd_trace):
            continue

        ssd_randoms_histogram.append(ssd_max)

        # fig, (ax1, ax2) = plt.subplots(2, 1)
        # ax1.axvline(latch_bin // 3)
        # ax1.plot(range(682), wcd[0])
        # ax1.plot(range(682), wcd[1])
        # ax1.plot(range(682), wcd[2])
        # ax1.set_xlim(0, 682)

        # ax2.plot(range(latch_bin - 20, latch_bin + 49), ssd_trace_window)
        # ax2.set_xlim(0, 2048)
        # fig.savefig(f'{i_trace}_window_cut.png')

        # break
    # break

np.savetxt("T1_coinc_histo.txt", np.array(ssd_randoms_histogram))
n, bins, _ = plt.hist(ssd_randoms_histogram)
plt.plot(0.5 * (bins[1:] + bins[:-1]), n / (n_analyzed_files * 5000 * 2048 * 8.33e-9))

plt.yscale("log")
plt.savefig("ssd_histogram.png")
