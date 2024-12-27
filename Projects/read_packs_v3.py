import matplotlib.pyplot as plt
from Histo import SdHisto
from Monit import Monit
import numpy as np
import signal

from matplotlib.colors import TABLEAU_COLORS
from matplotlib import dates
from datetime import datetime

to_dt = lambda ts: [datetime.fromtimestamp(t) for t in ts]

monit = Monit(2024, 12, 26)
monit.keys()

# offline calib
histos = np.loadtxt('peak.txt')
histos = np.split(histos, len(histos)//4)

peak_times = {
        943: [],
        944: [],
        949: [],
        954: [],
        }

peak_values = {
        943: [],
        944: [],
        949: [],
        954: [],
        }

for histo in histos:
    station_id = int(histo[:, 0][0])
    daq_time   = histo[:, 1][0]
    timestamp  = int(histo[:, 2][0])
    # tube_mask  = histo[:, 3][0]
    iPMT = histo[:, 3]

    if station_id in [945, 972, 948]: continue

    PeakHisto = SdHisto(peak=histo[:, 4:])
    try:
        peak_charge = PeakHisto()

        peak_times[station_id].append(timestamp)
        peak_values[station_id].append(peak_charge['peak'][-1])
    except KeyboardInterrupt:
        fig = PeakHisto.plot()
        fig.savefig(f"{station_id}_{timestamp}.png")
        continue

fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True)
colors = iter(TABLEAU_COLORS)

# online calib
for _id in [943, 944, 949, 954]:
    
    time = monit('fTime', _id)
    peak = monit('fPeak[4]', _id)

    c = next(colors)

    ax1.plot(to_dt(time[1:]), peak[1:, -1], color=c)
    ax1.errorbar(to_dt(peak_times[_id]), [x.n for x in peak_values[_id]], 
                 yerr=[x.std_dev for x in peak_values[_id]],
                 ls='none', color=c, fmt='o')

    min_index = [np.argmin(np.abs(t - time)) for t in peak_times[_id]]
    diff = (np.array([x.n for x in peak_values[_id]]) - peak[min_index, -1]) / peak[min_index, -1]
    ax2.plot(to_dt(time[min_index]), diff, color=c, label=f"{_id}: {np.mean(diff)*100:+.1f}%")

ax1.errorbar([], [], fmt='ko', label='from T3')
ax1.plot([], [], c='k', label='from monit')
ax2.axhline(0, c='k', ls='--')

ax1.legend()
ax2.legend()

locator = dates.AutoDateLocator()
formatter = dates.ConciseDateFormatter(locator)
ax2.xaxis.set_major_formatter(formatter)

fig.savefig('peaks_v3.png')
