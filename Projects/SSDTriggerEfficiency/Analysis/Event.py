import numpy as np

class Event():

    def __init__(self, file_path : str, cfg_class) -> None :

        data = np.loadtxt(file_path)

        self.energy = set(data[:, 0])
        self.zenith = set(data[:, 1])

        assert len(self.energy) == 1, f"Malformed event =(, got energy {self.energy}"
        assert len(self.zenith) == 1, f"Malformed event =(, got zenith {self.zenith}"
        self.energy, self.zenith = self.energy.pop(), self.zenith.pop()

        self.stations = []
        for count in range(0, len(data), 4):
            self.stations.append(Station(data[count : count+4, 2:], cfg_class))

    def __getitem__(self, station_id : int):

        for station in self.stations:
            if station_id == station.id: return station
        else:
            raise ValueError(f"Station {station_id} not found")


class Station():

    def __init__(self, station_data, cfg_class) -> None : 

        self.id = set(station_data[:, 0])
        self.spd = set(station_data[:, 1])
        self.cfg = cfg_class

        assert len(self.id) == 1, f"Malformed station =(, got id {self.id}"
        assert len(self.spd) == 1, f"Malformed station =(, got spd {self.id}"
        self.id, self.spd = self.id.pop(), self.spd.pop()

        self.wcd_traces = station_data[:-1, 3:]
        self.ssd = station_data[-1, 3:]

    def isT2(self) -> int : return self.isToT_WCD() or self.isToT_SSD()

    def isToT_SSD(self) -> int : 
        
        running_sum = (self.ssd[:120] > self.cfg.threshold['ssd']).sum()

        for i in range(120, len(self.ssd)):
            if running_sum > self.cfg.occupancy['ssd']: return i

            running_sum += (self.ssd[i      ] > self.cfg.threshold['ssd'])
            running_sum -= (self.ssd[i - 120] > self.cfg.threshold['ssd'])

        else:
            return 0
    
    def isToT_WCD(self) -> int :

        pmt_multiplicity_check = lambda sums : sum(sums > self.cfg.multiplicity['wcd']) > 1

        first_120_bins = self.wcd_traces[:, :120]
        pmt_running_sum = (first_120_bins >= self.cfg.threshold['wcd']).sum(axis=1)

        for i in range(120, self.wcd_traces.shape[1] + 1):
            if pmt_multiplicity_check(pmt_running_sum) : return i
            if i == self.wcd_traces.shape[1]: return 0

            new_over_threshold = np.array(self.wcd_traces[:, i] > self.cfg.multiplicity['wcd'], dtype=int)
            old_over_threshold = np.array(self.wcd_traces[:, i - 120] > self.cfg.multiplicity['wcd'], dtype=int)
            pmt_running_sum += new_over_threshold - old_over_threshold


        