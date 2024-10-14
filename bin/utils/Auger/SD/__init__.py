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

    monit_path = '/cr/auger02/Prod/monit/Sd/'
    monit_path2 = '/cr/data01/filip/Data/monit'

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
                if os.path.isfile(f"{self.monit_path}/{y:04}/{m:02}/mc_{y:04}_{m:02}_{d:02}_00h00.root"):
                    full_file_paths.append(f"{self.monit_path}/{y:04}/{m:02}/mc_{y:04}_{m:02}_{d:02}_00h00.root")
                elif os.path.isfile(f"{self.monit_path2}/mc_{y:04}_{m:02}_{d:02}_00h00.root"):
                    full_file_paths.append(f"{self.monit_path2}/mc_{y:04}_{m:02}_{d:02}_00h00.root")
                else:
                    self.logger.error(f"I cannot find the monit file for {y:04}-{m:02}-{d:02} !!!")
                    raise FileNotFoundError
        
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


class SdHisto:

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


    def fit(self) -> dict:

        if self.histos['peak'] is not None: 
            self.popts['peak'] = self.get_peak('peak')
        if self.histos['charge'] is not None:
            self.popts['charge'] = self.get_peak('charge')

        return self.popts


    def get_peak(self, mode: str) -> list[uncertainties.ufloat]:

        peaks = []
        for i, counts in enumerate(self.histos[mode]):

            bin_change = 99 if len(counts) == 150 else 399
            start, stop, peak = self.range_finder(counts[:bin_change], i, mode)
            bins = self.get_bins(mode, i)

            try:
                
                x1, x2, y1, y2 = start, bin_change, counts[stop], counts[bin_change]
                slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
                exp_fit = lambda x: np.exp(slope*(x-x1)+offset)
                exp_background = exp_fit(range(start, stop))
        
                popts, pcov = curve_fit(self.parabola, bins[start:stop], 
                                        counts[start:stop] - exp_background,
                                        bounds=([-np.inf, 0, 0],[0, np.inf, np.inf]),
                                        maxfev=10000, p0=[-0.01, bins[peak], counts[peak]],
                                        nan_policy='omit')
                
                # popts, pcov = curve_fit(self.exp_hump, bins[start:stop], counts[start:stop],
                #                         bounds=([0, 0, 0, 0, 0], [np.inf, np.inf, np.inf, np.inf, np.inf]),
                #                         maxfev=10000, p0=[5e3, 2e2, 0.1, 200 if mode=='peak' else 2000, 0.1]
                # )

                popts = uncertainties.correlated_values(popts, pcov)
                if mode == 'peak' and popts[1].n > 300: raise ValueError(f"calculated {popts[1]:i} ADC for Peak")
                if mode == 'charge' and popts[1].n > 3000: raise ValueError(f"calculated {popts[1]:i} ADC for Peak")
                if popts[1].std_dev > 0.1 * popts[1].n: raise ValueError("reject due to large fit error")
                
                peaks.append(popts)

            except Exception as e:
                # print(f"PMT {i} fit failed: {e}")
                peaks.append([uncertainties.ufloat(np.nan, np.nan) for _ in range(3)])
                # raise e

            self.popts[mode][i] = peaks
        
        return peaks
    

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
                ax1.plot(self.get_bins('peak', i) * factor, counts, c=c[i], ls='-', label=l[i])
                ax1.axvline(self.popts['peak'][i][1].n * factor, ymin=0.95, c=c[i])
            
            ax1.set_xlim(0, 400)
            ax1.legend(title='Peak')

        if self.histos['charge'] is not None:
            ax2.set_xlabel('integral / ADC')
            for i, counts in enumerate(self.histos['charge']):
                factor = 1 if i<3 else f
                ax2.plot(self.get_bins('charge', i) * factor, counts, c=c[i], ls='-', label=l[i])
                ax2.axvline(self.popts['charge'][i][1].n * factor, ymin=0.95, c=c[i])
            
            ax2.set_xlim(0, 3200)
            ax2.legend(title='Charge')

        return fig
    

    @staticmethod
    def range_finder(counts: np.ndarray, pmt: int, mode: str) -> tuple[int]:
        
        if pmt<3:
            peak = np.argmax(counts)
            x1, x2, y1, y2 = peak, len(counts), counts[peak], counts[len(counts)-1]

            slope, offset = -(np.log(y1)-np.log(y2))/(x2-x1), np.log(y1)
            exp_fit = lambda x: np.exp(slope*(x-x1)+offset)

            guess = np.argmax(counts[peak:] - exp_fit(range(peak, len(counts))))
            start = guess - (6 if mode=='peak' else 18)
            stop = guess + (25 if mode=='peak' else 100)

        else:
            peak = np.argmax(counts)
            skip = 7 if mode=='peak' else 18
            guess = peak + skip + np.argmax(counts[peak + skip:])
            start = guess - (5 if mode=='peak' else 18)
            stop = guess + (18 if mode=='peak' else 40)

        return start, stop, guess


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
