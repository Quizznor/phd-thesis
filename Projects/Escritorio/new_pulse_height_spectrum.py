from utils.binaries import *
from utils.Auger.SD.UubRandoms import UubRandom

STATION = 'Svenja'
total_time = 0
spectrum = np.zeros(1000)

for File in tools.ProgressBar(UubRandom(f"{STATION}", "ssd"), newline=False):
    
    traces = File["trace"]

    for trace in traces:
        total_time += 2048 * 8.33e-9
        index_up_to = np.max(trace) + 1
        if index_up_to < 70: continue
        spectrum[:index_up_to] += 1

spectrum_rate = spectrum / total_time
print(f"total time was {total_time:.2f} seconds")
print(f"histo MIP peak was {trace['mip_peak']:.2f}")
np.savetxt(f"{STATION}_full_bandwidth_peak_height_spectrum.txt", spectrum_rate)
