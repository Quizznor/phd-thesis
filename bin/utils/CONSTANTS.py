from dataclasses import dataclass

@dataclass
class CONSTANTS():

    import numpy as np

    AUGER_FD_ROOT : str = '/cr/users/filip/bin/utils/Auger/FD'
    GPS_OFFSET : int = 315964800

    # Auger SD Station specifications
    # from Framework/SDetector/Station.h
    UUB_WCD_PEAK_EDGES: np.ndarray = np.array([4*k for k in range(100)] + [400 + 16*k for k in range(51)])
    UUB_WCD_CHARGE_EDGES: np.ndarray = np.array([8*k for k in range(400)] + [3200 + 32*k for k in range(201)])
    UUB_WCD_PEAK: np.ndarray = 0.5 * (UUB_WCD_PEAK_EDGES[1:] + UUB_WCD_PEAK_EDGES[:-1])
    UUB_WCD_CHARGE: np.ndarray = 0.5 * (UUB_WCD_CHARGE_EDGES[1:] + UUB_WCD_CHARGE_EDGES[:-1])

    UUB_SSD_PEAK_EDGES: np.ndarray = np.array([2*k for k in range(100)] + [200 +  8*k for k in range(51)])
    UUB_SSD_CHARGE_EDGES: np.ndarray = np.array([2*k for k in range(400)] + [800 + 8*k for k in range(201)])
    UUB_SSD_PEAK: np.ndarray = 0.5 * (UUB_SSD_PEAK_EDGES[1:] + UUB_SSD_PEAK_EDGES[:-1])
    UUB_SSD_CHARGE: np.ndarray = 0.5 * (UUB_SSD_CHARGE_EDGES[1:] + UUB_SSD_CHARGE_EDGES[:-1])