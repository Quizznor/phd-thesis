import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import CONSTANTS
import scienceplots


matplotlib.use('TkAgg')
plt.style.use(['science', 'ieee', 'no-latex'])


vertical, vertical_err = np.zeros((2, 80))
omnidirectional, omnidirectional_err = np.zeros((2, 400))

def read(line: str) -> tuple[int, float]:
    idx, val = line.split(",")
    idx = idx.split("(")[-1]
    val = val.split(")")[0]

    return int(idx) - 2, float(val)


with open("unnamed.c.C", "r") as f:
    lines = f.readlines()

for line in lines:
    if line.startswith("   hChargeSSD__1->SetBinContent"):
        idx, val = read(line.rstrip())
        vertical[idx] = val
    elif line.startswith("   hChargeSSD__1->SetBinError"):
        idx, val = read(line.rstrip())
        vertical_err[idx] = val
    elif line.startswith("   hCalibChargeSSD__2->SetBinContent"):
        idx, val = read(line.rstrip())
        omnidirectional[idx] = val
    elif line.startswith("   hCalibChargeSSD__2->SetBinError"):
        idx, val = read(line.rstrip())
        omnidirectional_err[idx] = val

plt.rcParams['figure.figsize'] = [3.3, 2.5]

bins_vertical = np.linspace(0, 800, 80)
#bins_omnidirectional = np.linspace(0, 800, 400)

#f = 1
#plt.plot(bins_omnidirectional, f * omnidirectional, label="all events (norm.)")
#plt.fill_between(
#    bins_omnidirectional,
#    f * (omnidirectional - omnidirectional_err),
#    f * (omnidirectional + omnidirectional_err),
#    alpha=0.2,
#)

plt.plot(bins_vertical, vertical, label="vertical only")
plt.fill_between(
    bins_vertical, vertical - vertical_err, vertical + vertical_err, alpha=0.2
)

plt.xlabel("sum / ADC")
plt.ylabel("Occurences")
plt.xlim(0, 500)
plt.legend()

plt.savefig('charge.png')


peak = np.loadtxt('peak.txt')
peak = peak[3::4, 4:]

plt.figure()
plt.plot(CONSTANTS.UUB_SSD_PEAK, peak[2], lw=0.8)
plt.fill_between(CONSTANTS.UUB_SSD_PEAK, peak[2] - np.sqrt(peak[2]),
                 peak[2] + np.sqrt(peak[2]), color='k', alpha=0.2,
                 edgecolor='none')
plt.xlabel('max. pulse height / ADC')
plt.ylabel('Occurences')

plt.xlim(0, 150)
plt.ylim(0, 1500)

plt.savefig('peak.png')
