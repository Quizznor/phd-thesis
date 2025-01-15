# import matplotlib.pyplot as plt
# import matplotlib.style
# from Histo import SdHisto
# from Monit import Monit
# import numpy as np
# import signal
# import matplotlib

from utils.binaries import *
from utils.plotting import *
from utils.Auger.SD import *
from datetime import datetime



# from matplotlib.colors import TABLEAU_COLORS
# from matplotlib import dates
# from datetime import datetime

# matplotlib.use('TkAgg')
# import scienceplots

# matplotlib.style.use(['science', 'ieee', 'no-latex'])

CORR_FACTOR = 1.21875

to_dt = lambda ts: [datetime.fromtimestamp(t) for t in ts]

monit = Monit(2025, 1, range(7, 11))

# offline calib
histos = np.loadtxt('peak.txt')
histos = np.split(histos, len(histos)//4)

peak_times = {
        609: [],
        943: [],
        944: [],
        949: [],
        954: [],
        }

peak_values = {
        609: [],
        943: [],
        944: [],
        949: [],
        954: [],
        }

for i, histo in enumerate(histos):
    station_id = int(histo[:, 0][0])
    daq_time   = histo[:, 1][0]
    timestamp  = int(histo[:, 2][0])
    tube_mask  = histo[:, 3][0]
    iPMT = histo[:, 4]

    if station_id in [945, 972, 948]: continue

    PeakHisto = SdHisto(peak=histo[:, 5:], pmt_mask=[0,0,0,1])
    try:
        print(f'{i:03} ping...', end='', flush=True)
        peak_charge = PeakHisto()
        print('pong!')

        peak_times[station_id].append(timestamp)
        peak_values[station_id].append(peak_charge['peak'][-1])
        
        #fig = PeakHisto.plot()
        #fig.savefig(f"success/{station_id}_{timestamp}.png")
        #plt.clf()

    except KeyboardInterrupt:
        print('Thanks')
        fig = PeakHisto.plot()
        fig.savefig(f"fail/{station_id}_{timestamp}.png")
        plt.clf()
        continue

fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True)
# colors = iter(TABLEAU_COLORS)

# online calib
for _id in [943, 944, 949, 954]:
    
    time = monit('fTime', _id)
    peak = monit('fPeak[4]', _id)
    peak = peak[:, -1] / CORR_FACTOR

    c = 'k' #next(colors)

    ax1.plot(to_dt(time[1:]), peak[1:], color=c, marker='none')
    ax1.errorbar(to_dt(peak_times[_id]), [x.n for x in peak_values[_id]], 
                 yerr=[x.std_dev for x in peak_values[_id]],
                 ls='none', color=c, fmt='o', markersize=1)

    min_index = [np.argmin(np.abs(t - time)) for t in peak_times[_id]]
    diff = (np.array([x.n for x in peak_values[_id]]) - peak[min_index]) / peak[min_index]

    # 1000s between events w/ running mean over 86 events = 23.89 hours
    # ax2.plot(to_dt(time[min_index])[43:-42], np.convolve(diff, np.ones(86)/86, mode='valid'), color=c, label=f"{_id}: {np.nanmean(diff)*100:+.1f}%")
    ax2.plot(to_dt(time[min_index]), diff, color=c, label=f"{_id}: {np.nanmean(diff)*100:+.1f}%", marker='none')

ax1.errorbar([], [], fmt='ko', label='from T3')
ax1.plot([], [], c='k', label='from monit')
ax2.axhline(0, c='k', ls='--')

ax1.legend()
ax2.legend()

plot.apply_datetime_format(ax2)

xmin, xmax = ax2.get_xlim()
ax2.set_xlim(xmin, xmax + 0.4 * (xmax-xmin))

fig.savefig('peaks_v3.png')
