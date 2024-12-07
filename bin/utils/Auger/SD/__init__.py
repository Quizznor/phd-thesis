from ... import logging, create_stream_logger
from ...binaries import uncertainties
from scipy.signal import argrelextrema
from scipy.optimize import curve_fit
from itertools import product
from ...plotting import plt
from ...binaries import np
from ... import CONSTANTS
import uproot
import typing
import json
import os


class Monit():

    monit_paths = [
        '/cr/auger02/Prod/monit/Sd/',           # Mirror to Lyon DB
        '/cr/data01/filip/Data/monit/',         # local repo @ IAP
        '/home/filip/Desktop/monit_and_sd/',    # local repo @ Debian12
    ]

    def __init__(self, *args, starting_branch=None, verbosity=logging.INFO) -> None :

        starting_branch = starting_branch or "SDMonCal/SDMonCalBranch"
        self.logger = create_stream_logger("SD.Monitor", loglevel=verbosity)
        
        if isinstance(args[0], str):
            full_file_paths = args
        else:
            years, months, days = args
            if isinstance(years, int): years = [years]
            if isinstance(months, int): months = [months]
            if isinstance(days, int): days = [days]

            full_file_paths = []
            for y, m, d in product(years, months, days):
                for path in self.monit_paths:
                    if os.path.isfile(f"{path}/{y:04}/{m:02}/mc_{y:04}_{m:02}_{d:02}_00h00.root"):
                        full_file_paths.append(f"{path}/{y:04}/{m:02}/mc_{y:04}_{m:02}_{d:02}_00h00.root")
                        break
                else:
                    self.logger.error(f"I cannot find the monit file for {y:04}-{m:02}-{d:02} !!!")
                    raise FileNotFoundError(f"mc_{y:04}_{m:02}_{d:02}_00h00.root not found in any data path you've specified")
        
        self.logger.info(f'received {len(full_file_paths)} file(s) as input')

        """
        opening individual files is faster than concatenate, iterate etc.,
        because we dont immediately load everything into memory at once
        """
        self.__streams = [uproot.open(f"{file}:{starting_branch}") for file in full_file_paths]

        """these keys surely generalize to the entire dataset..."""
        temp, self._keys = self.__streams[0].keys(), {}
        temp.sort(key=lambda x: x.count('/'))

        for key in temp:
            try:
                branch, name = key.split('/')
                self._keys[branch][name.split('.')[-1].replace('[3]', '')] = key

            except (ValueError, KeyError):
                if key in ['fLsId', 'fTime', 'fCDASTime']: 
                    self._keys[key] = key
                else : 
                    self._keys[key] = {}

        self.logger.info(f"View monit keys with self.keys()")

    def __getitem__(self, item) -> typing.Union[dict, str]:
        return self.get_key(item) or self._keys[item]
    
    def __call__(self, path, station : int = -1) -> np.ndarray :
        """Fetching multiple stations is discouraged due to runtime performance"""
        result = []

        easy_path = self.get_key(path)
        full_path = path if easy_path is None else easy_path

        for stream in self.__streams:
            data = stream[full_path].array()

            if station != -1:
                station_list = stream['fLsId'].array()
                station_mask = station_list == station
            else: station_mask = [True for _ in range(len(data))]

            [result.append(x) for x in data[station_mask]]

        maybe_station = f' and station #{station}' if station != -1 else ''
        self.logger.info(f"found {len(result)} entries for key {path}{maybe_station}")  
        return np.array(result)

    def get_key(self, key) -> typing.Union[None, str] :
        for branch in ['fMonitoring', 'fRawMonitoring', 'fCalibration']:
            try:
                return self._keys[branch][key]
            except KeyError:
                continue

    def keys(self) -> typing.NoReturn :
        print(json.dumps(self._keys, indent=2))


class SdHisto():
    def __init__(self, *, peak: list[np.ndarray]=None, charge: list[np.ndarray]=None) -> None:

        assert peak is None or len(peak) == 4, 'Missing a PMT?'
        assert charge is None or len(charge) == 4, 'Missing a PMT?'
        
        self.histos = {
            'peak' : peak,
            'charge' : charge
        }
        self.popts = {
            'peak': [[np.nan, np.nan] for _ in range(4)], 
            'charge': [[np.nan, np.nan] for _ in range(4)]
        }

        self.fit_was_run = False


    def __call__(self) -> list[uncertainties.ufloat]:
        if not self.fit_was_run:
            _ = self.fit()

        peak_charge = {
            'peak': [],
            'charge': [],
        }
        for i in range(4):
            peak_charge['peak'].append(self.popts['peak'][i][1])
            peak_charge['charge'].append(self.popts['charge'][i][1])

        return peak_charge

    def fit(self) -> dict:

        if self.histos['peak'] is not None: 
            self.popts['peak'] = self.get_peak('peak')
        if self.histos['charge'] is not None:
            self.popts['charge'] = self.get_peak('charge')

        self.fit_was_run = True

        return self.popts


    def get_peak(self, mode: str) -> list[list[uncertainties.ufloat]]:

        peaks = []
        for i, counts in enumerate(self.histos[mode]):

            if i < 3: peaks.append(self.fit_wcd(counts[:(99 if mode=='peak' else 399)]))
            else: peaks.append(self.fit_ssd(counts[:(99 if mode=='peak' else 399)]))

        return peaks
    

    @staticmethod
    def fit_wcd(counts: np.ndarray) -> list[uncertainties.ufloat]:

        try:
            match len(counts):
                case  99: 
                    increment = 5
                    bins = CONSTANTS.UUB_WCD_PEAK
                    initial_start = 99 - increment
                case 399: 
                    increment = 20
                    bins = CONSTANTS.UUB_WCD_CHARGE
                    initial_start = 399 - increment
                case _:
                    raise IndexError(f'received histogram with length {len(counts)}')

            old_peak, guess = np.argmax(counts[initial_start:]), 0
            while old_peak != guess:
                old_peak = guess
                initial_start -= increment
                guess = np.argmax(counts[initial_start:]) + initial_start

            start, stop = guess - increment, guess + increment
            x1, x2, y1, y2 = start, len(counts)-1, counts[stop], counts[len(counts)-1]
            background_slope = lambda x: (y2-y1)/(x2-x1)*(x-x1) + y1
    
            popts, pcov = curve_fit(SdHisto.parabola, bins[start:stop], 
                                    np.log(counts[start:stop]) - background_slope(np.arange(start, stop, 1)),
                                    bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                    maxfev=100000, p0=[-0.01, bins[guess], counts[guess]],
                                    nan_policy='omit')

            popts = uncertainties.correlated_values(popts, pcov)
            # if len(counts) == 99 and popts[1].n < 100 or 300 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for WCD peak")
            # if len(counts) == 399 and popts[1].n < 1000 or 2000 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for WCD charge")
            if (r := popts[1].std_dev / popts[1].n) > 0.2: raise ValueError(f"large fit error for WCD: {r*100:.0f}%")
            return popts

        except Exception as e:
            # print(f'WCD SdHisto fit failed: {e}')
            return [uncertainties.ufloat(np.nan, np.nan) for _ in range(3)]


    @staticmethod
    def fit_ssd(counts: np.ndarray) -> list[uncertainties.ufloat]:

        try:
            match len(counts):
                case  99: 
                    bins = CONSTANTS.UUB_SSD_PEAK
                    increment = 5
                    start = np.argmax(counts)

                    while not np.argmax(counts[start:]):
                        start += 1
                case 399: 
                    bins = CONSTANTS.UUB_SSD_CHARGE
                    increment = 20
                    order = 10

                    while len((dips := argrelextrema(counts[1:], np.less, order=order)[0])) > 1:
                        order += 1
                    start = dips[0] + 1

                case _:
                    raise IndexError(f'received histogram with length {len(counts)}')


            guess = start + np.argmax(counts[start:])
            start, stop = guess - increment, guess + increment

            popts, pcov = curve_fit(SdHisto.parabola, bins[start:stop], counts[start:stop],
                                    bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                    maxfev=100000, p0=[-0.01, bins[guess], counts[guess]],
                                    nan_policy='omit')

            popts = uncertainties.correlated_values(popts, pcov)
            # if len(counts) == 99 and popts[1].n < 20 or 100 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for SSD peak")
            # if len(counts) == 399 and popts[1].n < 20 or 100 < popts[1].n: raise ValueError(f"calculated {popts[1]:i} ADC for SSD charge")
            if (r := popts[1].std_dev / popts[1].n) > 0.2: raise ValueError(f"large fit error for SSD: {r*100:.0f}%")
            return popts

        except Exception as e:
            # print(f'SSD SdHisto fit failed: {e}')
            return [uncertainties.ufloat(np.nan, np.nan) for _ in range(4)]
    

    def plot(self) -> plt.Figure:
        
        if self.histos['peak'] is not None and self.histos['charge'] is not None:
            fig, (ax1, ax2) = plt.subplots(1, 2)
        else:
            fig, ax = plt.subplots()
            ax1 = ax2 = ax

        f = 4
        c = ['red', 'blue', 'mediumturquoise', 'k']
        l = ['WCD1', 'WCD2', 'WCD3', rf'SSD $\times$ {f}']
        
        if self.histos['peak'] is not None:
            ax1.set_xlabel('max. pulse height / ADC')
            for i, counts in enumerate(self.histos['peak']):
                factor = 1 if i<3 else f
                ax1.plot(self.get_bins('peak', i) * factor, counts, c=c[i], ls='-', label=l[i], marker='none')
                ax1.axvline(self.popts['peak'][i][1].n * factor, lw=0.4, ls='--', c=c[i])
                err = self.popts['peak'][i][1].std_dev * np.array([-1, 1]) + self.popts['peak'][i][1].n
                ax1.axvspan(*(err * factor), color=c[i], alpha=0.1)

            ax1.set_xlim(0, 400)
            ax1.legend(title='Peak')

        if self.histos['charge'] is not None:
            ax2.set_xlabel('integral / ADC')
            for i, counts in enumerate(self.histos['charge']):
                factor = 1 if i<3 else f
                ax2.plot(self.get_bins('charge', i) * factor, counts, c=c[i], ls='-', label=l[i], marker='none')
                ax2.axvline(self.popts['charge'][i][1].n * factor, lw=0.4, ls='--', c=c[i])
                err = self.popts['charge'][i][1].std_dev * np.array([-1, 1]) + self.popts['charge'][i][1].n
                ax2.axvspan(*(err * factor), color=c[i], alpha=0.1)
            
            ax2.set_xlim(0, 3200)
            ax2.legend(title='Charge')

        return fig
    

    @staticmethod
    def parabola(x, scale, mip, y0):
        return scale * (x-mip)**2 + y0


    @staticmethod
    def get_bins(mode: str, pmt: int) -> np.ndarray:

        if mode == 'peak' and pmt < 3:
            return CONSTANTS.UUB_WCD_PEAK
        elif mode == 'peak' and pmt == 3:
            return CONSTANTS.UUB_SSD_PEAK
        elif mode == 'charge' and pmt < 3:
            return CONSTANTS.UUB_WCD_CHARGE
        elif mode == 'charge' and pmt == 3:
            return CONSTANTS.UUB_SSD_CHARGE
        

def read_histos(path_to_file: str) -> list[dict]:
    
    return_list = []

    original_data = np.loadtxt(path_to_file, dtype=int)
    
    for event in np.split(original_data, len(original_data) // 4):

        assert len(id := np.unique(event[:, 0])) == 1, "Missing data =( (Station Id looks wrong)" 
        assert len(daq_time := np.unique(event[:, 1])) == 1, "Missing data =( (DAQ time looks wrong)" 
        assert len(timestamp := np.unique(event[:, 2])) == 1, "Missing data =( (timestamp looks wrong)"

        return_list.append({
            'id': id[0],
            'daq_time': daq_time[0],
            'timestamp': timestamp[0],
            'data': event[:, 4:]
        })

    return return_list