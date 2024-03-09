from ... import logging, create_stream_logger
from itertools import product
from ...binaries import np
import uproot
import typing
import json
import os

class Monit():

    monit_path = '/cr/auger02/Prod/monit/Sd/'

    def __init__(self, years : list[int], months : list[int], days : list[int], /, *, starting_branch=None, verbosity=logging.INFO) -> None :

        starting_branch = starting_branch or "SDMonCal/SDMonCalBranch"
        if isinstance(years, int): years = [years]
        if isinstance(months, int): months = [months]
        if isinstance(days, int): days = [days]
        
        self.logger = create_stream_logger("SD.Monitor", loglevel=verbosity)
        to_path = lambda y, m, d : f"{self.monit_path}/{y:04}/{m:02}/mc_{y:04}_{m:02}_{d:02}_00h00.root"
        files = [to_path(YYYY, MM, DD) for YYYY, MM, DD in product(years, months, days)]
        self.logger.info(f'received {len(files)} file(s) as input')
        temp = set([file for file in files if not os.path.isfile(file)])
        for file in temp: self.logger.warning(f"I cannot find {file}!")
        
        """
        opening individual files is faster than concatenate, iterate etc.,
        because we dont immediately load everything into memory at once
        """
        self.__streams = [uproot.open(f"{file}:{starting_branch}") for file in set(files) - temp]

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

    def keys(self) -> str :
        return json.dumps(self._keys, indent=2)