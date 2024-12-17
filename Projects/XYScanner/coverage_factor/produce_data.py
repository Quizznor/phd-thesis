import sys, os
import numpy as np

BASE = "/home/filip/xy-calibration/"
RUNNO = "12822"

coverage = [2.35, 2.47, 2.60]

os.chdir(BASE)
for c in coverage:
    input(f"change area to: {c}")

    c = np.round(c, 2)
    os.system(f"{BASE}/run_Calib.py -r -i {RUNNO}")
    os.system(
        f"mv {BASE}/results/outCorr_{RUNNO}.txt /home/filip/Desktop/phd-thesis/Projects/XYScanner/coverage_factor/outCorr_{RUNNO}_{c}.txt"
    )
