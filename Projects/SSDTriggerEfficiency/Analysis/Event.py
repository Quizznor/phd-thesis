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

    def isT2(self) -> bool : return self.isT2WCD() or self.isT2SSD()

    def isT2SSD(self) -> bool : return self.isToT(self.ssd, 'ssd')
    
    def isT2WCD(self) -> bool :

        multiplicity = (self.isToT(wcd_trace, 'wcd') for wcd_trace in self.wcd_traces)
        return sum(multiplicity) > self.cfg.multiplicity['wcd']
    

    def isToT(self, trace : np.ndarray, pmt : str) -> bool : 

        running_sum = (trace[:120] > self.cfg.threshold[pmt]).sum()

        for i in range(120, len(trace)):
            if running_sum > self.cfg.occupancy[pmt]: return True

            running_sum += (trace[i      ] > self.cfg.threshold[pmt])
            running_sum -= (trace[i - 120] > self.cfg.threshold[pmt])

        else:
            return False