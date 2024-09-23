import numpy as np

class Event():

    def __init__(self, file_path : str) -> None :

        data = np.loadtxt(file_path)

        self.energy = set(data[:, 0])
        self.zenith = set(data[:, 1])

        assert len(self.energy) == 1, f"Malformed event =(, got energy {self.energy}"
        assert len(self.zenith) == 1, f"Malformed event =(, got zenith {self.zenith}"
        self.energy, self.zenith = self.energy.pop(), self.zenith.pop()

        self.stations = []
        for count in range(0, len(data), 4):
            self.stations.append(Station(data[count : count+4, 2:]))

    def __getitem__(self, station_id : int):

        for station in self.stations:
            if station_id == station.id: return station
        else:
            raise ValueError(f"Station {station_id} not found")
        
    
    def __str__(self) -> str :
        return_str = ""

        for station in self.stations:
            return_str += station.__str__() + "\n"

        return return_str

    def __len__(self) -> int :
        return len(self.stations) 
    
    def get_ids(self) -> list[int] :
        return [station.id for station in self.stations]
    
    
    def get_spds(self) -> list[int] :
        return [station.spd for station in self.stations]


class Station():

    def __init__(self, station_data) -> None : 

        self.id = set(station_data[:, 0])
        self.spd = set(station_data[:, 1])

        assert len(self.id) == 1, f"Malformed station =(, got id {self.id}"
        assert len(self.spd) == 1, f"Malformed station =(, got spd {self.id}"
        self.id, self.spd = int(self.id.pop()), int(self.spd.pop())

        self.wcd_traces = station_data[:-1, 3:]
        self.ssd = station_data[-1, 3:]

    def isWCDT2(self, cfg) -> int : return self.isTh_WCD(cfg) or self.isToT_WCD(cfg)

    def isToT_SSD(self, cfg) -> int : 
        
        running_sum = (self.ssd[:120] > cfg.threshold['ssd_tot']).sum()

        for i in range(120, len(self.ssd)):
            if running_sum > cfg.occupancy['ssd']: return i

            running_sum += (self.ssd[i      ] > cfg.threshold['ssd_tot'])
            running_sum -= (self.ssd[i - 120] > cfg.threshold['ssd_tot'])

        else:
            return -1


    def isTh_WCD(self, cfg) -> int :
        
        wcd1, wcd2, wcd3 = self.wcd_traces
        for i in range(2048):
            if wcd1[i] < cfg.threshold['wcd_th']: continue
            if wcd2[i] < cfg.threshold['wcd_th']: continue
            if wcd3[i] < cfg.threshold['wcd_th']: continue

            return i
        
        return -1


    def isToT_WCD(self, cfg) -> int :

        pmt_multiplicity_check = lambda sums : sum(sums > cfg.multiplicity['wcd']) > 1

        first_120_bins = self.wcd_traces[:, :120]
        pmt_running_sum = (first_120_bins >= cfg.threshold['wcd_tot']).sum(axis=1)

        for i in range(120, self.wcd_traces.shape[1] + 1):
            if pmt_multiplicity_check(pmt_running_sum) : return i
            if i == self.wcd_traces.shape[1]: return -1

            new_over_threshold = np.array(self.wcd_traces[:, i] > cfg.multiplicity['wcd'], dtype=int)
            old_over_threshold = np.array(self.wcd_traces[:, i - 120] > cfg.multiplicity['wcd'], dtype=int)
            pmt_running_sum += new_over_threshold - old_over_threshold


    def __str__(self) -> str :
        return f"Station {self.id} at {self.spd}m"    