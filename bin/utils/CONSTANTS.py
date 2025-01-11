from . import create_stream_logger
import numpy as np
import os

# backwards compatibility
AUGER_FD_ROOT: str = "/cr/users/filip/bin/utils/Auger/FD"

const_logger = create_stream_logger("const")

# fails if run on windows, but that's on you
path_set = True
match hostname := os.uname()[1]:
    case "debian12":
        raise NotImplementedError("Set up your machine!")
    case x if x.startswith("crc"):
        MONI_PATH: str = "/cr/work/filip/monit_and_sd/"
        HIST_PATH: str = "/cr/work/filip/monit_and_sd/"
        PLOT_PATH: str = "/cr/data01/filip/plots/"
        DATA_PATH: str = "/cr/data01/filip/Data/"
    case _:
        const_logger.error(f"pathspecs for {hostname} not found")
        path_set = False

if path_set:
    const_logger.info(f"set {MONI_PATH = }")
    const_logger.info(f"set {HIST_PATH = }")
    const_logger.info(f"set {PLOT_PATH = }")
    const_logger.info(f"set {DATA_PATH = }")

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
