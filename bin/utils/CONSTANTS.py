from . import create_stream_logger
from pathlib import Path
import numpy as np
import os

const_logger = create_stream_logger("const")

USERNAME = "filip"

# fails if run on windows, but that's on you
path_set = True
match hostname := os.uname()[1]:
    case "debian12":
        MONI_PATH: Path = Path("/home/filip/Data/monit_and_sd")
        HIST_PATH: Path = Path("/home/filip/Data/monit_and_sd")
        PLOT_PATH: Path = Path("/home/filip/Data/plots")
        DATA_PATH: Path = Path("/home/filip/Data/")
        SCAN_PATH: Path = Path("/home/filip/Public/xy-calibration")
        OFLN_PATH: Path = Path("/home/filip/Public/offline/install/")
    case x if x.startswith("crc"):
        MONI_PATH: Path = Path("/cr/work/filip/monit_and_sd")
        HIST_PATH: Path = Path("/cr/work/filip/monit_and_sd")
        PLOT_PATH: Path = Path("/cr/data01/filip/plots")
        DATA_PATH: Path = Path("/cr/data01/filip/Data")
        SCAN_PATH: Path = Path("/cr/data01/filip/xy-calibration")
        OFLN_PATH: Path = Path("/cr/data01/filip/offline/install/")
    case _:
        const_logger.error(f"pathspecs for {hostname} not found")
        path_set = False

if path_set:
    const_logger.info(f"set {MONI_PATH = }")
    const_logger.info(f"set {HIST_PATH = }")
    const_logger.info(f"set {PLOT_PATH = }")
    const_logger.info(f"set {DATA_PATH = }")
    const_logger.info(f"set {SCAN_PATH = }")
    const_logger.info(f"set {OFLN_PATH = }")

GPS_OFFSET: int = 315964800

# Auger SD Station specifications
# from Framework/SDetector/Station.h
class UUB:
    WCD_PEAK_EDGES: np.ndarray = np.array(
        [4 * k for k in range(100)] + [400 + 16 * k for k in range(51)]
    )
    WCD_CHARGE_EDGES: np.ndarray = np.array(
        [8 * k for k in range(400)] + [3200 + 32 * k for k in range(201)]
    )
    WCD_PEAK: np.ndarray = 0.5 * (WCD_PEAK_EDGES[1:] + WCD_PEAK_EDGES[:-1])
    UUB_WCD_CHARGE: np.ndarray = 0.5 * (
        WCD_CHARGE_EDGES[1:] + WCD_CHARGE_EDGES[:-1]
    )

    SSD_PEAK_EDGES: np.ndarray = np.array(
        [2 * k for k in range(100)] + [200 + 8 * k for k in range(51)]
    )
    SSD_CHARGE_EDGES: np.ndarray = np.array(
        [2 * k for k in range(400)] + [800 + 8 * k for k in range(201)]
    )
    SSD_PEAK: np.ndarray = 0.5 * (SSD_PEAK_EDGES[1:] + SSD_PEAK_EDGES[:-1])
    SSD_CHARGE: np.ndarray = 0.5 * (
        SSD_CHARGE_EDGES[1:] + SSD_CHARGE_EDGES[:-1]
    )

class WORD:
    SIM_HEADER = "\
# ************************************************************************************************\n\
#\
# PRAGUE LIBRARIES - PROTONS            # NAPOLI LIBRARIES - PHOTONS\n\
# 15_15.5 log E = 4999 Files            # 15_15.5 log E = 11250 Files\n\
# 15.5_16 log E = 4999 Files            # 15.5_16 log E = 11250 Files\n\
                                #_|     #                               #_|\n\
# 16_16.5 log E = 4999 Files    # | R   # 16_16.5 log E = 11250 Files   # | R\n\
# 16.5_17 log E = 4998 Files    # | E   # 16.5_17 log E = 14998 Files   # | E\n\
# 17_17.5 log E = 4999 Files    # | L   # 17_17.5 log E = 11250 Files   # | L\n\
# 17.5_18 log E = 4998 Files    # | E   # 17.5_18 log E = 20000 Files   # | E\n\
# 18_18.5 log E = 3123 Files    # | V   # 18_18.5 log E = 10000 Files   # | V\n\
                                # | A   #                               # | A\n\
# NAPOLI LIBRARIES - PROTONS    # | N   #                               # | N\n\
# 18.5_19 log E = 5012 Files    # | T   # 18.5_19 log E = 9990 Files    # | T\n\
# 19_19.5 log E = 5153 Files    # |     # 19_19.5 log E = 10000 Files   # |\n\
                                # |     #                               # |\n\
# 19.5_20 log E = 4997 Files            # 19.5_20 log E = 10000 Files\n\
# 20_20.2 log E = 2012 Files            # 20_20.2 log E = 10000 Files\n\
#\n\
# ************************************************************************************************"

    SIM_REQS = '\
Requirements        	=       OpSysName == "Ubuntu"               \\\n\
                        &&  OpSysMajorVer == 22                     \\\n\
                        && TARGET.Machine != "crc2.ikp.kit.edu"     \\\n\
                        && TARGET.Machine != "crc1.ikp.kit.edu"     \\\n\
                        && TARGET.Machine != "crcds99.iap.kit.edu"'

    RUN_PY_HEADER = "\
#!/usr/bin/python3\n\
\n\
import os, sys\n\
import subprocess\n\
from pathlib import Path\n\
\n\
is_sim_file = (lambda s: s.startswith(\"DAT\")\n\
                and not s.endswith(\".long\")\n\
                and not s.endswith(\".lst\")\n\
                and not s.endswith(\".gz\"))\n\
\n\
def prepare_bootstrap(name, src, out, i, r, n) -> bool:\n\
    files = list(filter(is_sim_file, os.listdir(src)))\n\
    f = Path(f\"{src}/{files[i]}\")\n\
    \n\
    replace = {\n\
        \"'@INPUTFILE@'\": str(f),\n\
        \"'@NPARTICLES@'\": str(n),\n\
        \"'@DETECTORSEED@'\": f\"{r:06}\",\n\
        \"'@PHYSICSSEED@'\": f\"{r+1:06}\",\n\
        \"'@OUTPUTFILE@'\": f\"{name}/{out}/{files[i]}_{r:02}.root\",\n\
        \"'@PATTERNPATH@'\": \"*.part\" if f.name.endswith(\".part\") else \"*\",\n\
        \"'@GROUNDDATA@'\": \"(1).part\" if f.name.endswith(\".part\") else \"(1)\",\n\
    }\n\
    \n\
    b_src = f\"{name}/src/bootstrap.xml\"\n\
    b_out = f\"{name}/sim/bootstrap_{out[4:].replace('/','_')}_{f.name}_{r:06}.xml\"\n\
    \n\
    with open(b_out, \"w\") as target:\n\
        with open(b_src, \"r\") as source:\n\
            for line in source.readlines():\n\
                try:\n\
                    target.write(replace[line.strip()] + \"\\n\")\n\
                except KeyError:\n\
                    target.write(line)\n\
    \n\
    return b_out\n\n"

    RUN_PY_FOOTER = "\
i = int(sys.argv[1])\n\
\n\
for r in range(0, RETHROWS):\n\
    if target_bootstrap := prepare_bootstrap(NAME, SRC, OUT, i, r, n):\n\
        subprocess.run([f\"{NAME}/work/{OUT[4:].replace('/','_')}/run.sh\", target_bootstrap])"