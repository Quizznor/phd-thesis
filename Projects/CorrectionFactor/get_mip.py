from utils.binaries import *
from utils.Auger.SD import *

from datetime import date, timedelta
from pathlib import Path
import os

CORR_FACTOR = 1.21875
start_dt = date(2025, 1, 14)
end_dt = date(2025, 1, 14)

stations = [609, 943, 944, 949, 954]

def daterange(date1, date2):
    for n in range(int ((date2 - date1).days)+1):
        yield date1 + timedelta(n)

LAST = os.getcwd()
histo_info = read_histos("peak.txt")

for d in daterange(start_dt, end_dt):

    print(f"Analyzing {d}")
    year, month, day = d.year, d.month, d.day

    date_fits = [d <= date.fromtimestamp(event['timestamp']) 
                <= (d + timedelta(1)) for event in histo_info]

    moni = Monit(year, month, day)

    path = f'/cr/work/filip/weather_correlation/'
    Path(path).mkdir(parents=True, exist_ok=True)

    with open(f"{path}/peaks_{year}_{month:02d}_{day:02d}.dat", 'w') as file:
        for station in stations:
            
            moni_times = moni('fTime', station)
            if not len(moni_times): continue        # 609 was disconnected on the 2025/01/10 

            moni_peaks = moni('fPeak[4]', station)[:,-1] / CORR_FACTOR
            station_fits = [event['id'] == station for event in histo_info]

            mask = np.logical_and(station_fits, date_fits)

            for candidate, event in zip(mask, histo_info):
                if not candidate: continue

                histo = SdHisto(peak = event['data'], pmt_mask=[0,0,0,1])
                mip = histo()['peak'][-1]

                closest_moni_index = tools.closest(moni_times, event['timestamp'], index=True)
                diff = (moni_peaks[closest_moni_index] - mip) / mip

                file.write(f"{station} {event['timestamp']} {mip.n} {diff.n} {diff.std_dev}\n")