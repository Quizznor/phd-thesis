from utils.binaries import *
from utils.Auger.SD.UubRandoms import UubRandom

STATION = 'NuriaJr'
total_time = 0
spectrum = np.zeros(1000)

for File in tools.ProgressBar(UubRandom(f"{STATION}", "ssd"), newline=False):
    
    trace = File["trace"]
    total_time += 2048 * 8.33e-9
    
    index_up_to = np.max(trace) + 1
    spectrum[:index_up_to] += 1

spectrum_rate = spectrum / total_time
np.savetxt(f"{STATION}_full_bandwidth_peak_height_spectrum.txt", spectrum_rate)
