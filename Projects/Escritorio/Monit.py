from itertools import product
import matplotlib.pyplot as plt
import numpy as np
import uproot
import typing
import glob
import json
import os


class Monit:

    monit_paths = [
        "/cr/auger02/Prod/monit/Sd/",  # Mirror to Lyon DB
        "/cr/data01/filip/Data/monit/",  # local repo @ IAP
        "/home/filip/Desktop/monit_and_sd/",  # local repo @ Debian12
        "/home/shifter/monit_and_sd/",  # local repo @ Malargue temp laptop
    ]

    def __init__(self, *args, starting_branch=None) -> None:

        starting_branch = starting_branch or "SDMonCal/SDMonCalBranch"
        # self.logger = create_stream_logger("SD.Monitor", loglevel=verbosity)

        if isinstance(args[0], str):
            full_file_paths = args
        else:
            years, months, days = args
            if isinstance(years, int):
                years = [years]
            if isinstance(months, int):
                months = [months]
            if isinstance(days, int):
                days = [days]

            full_file_paths = []
            for y, m, d in product(years, months, days):
                for path in self.monit_paths:
                    candidates = glob.glob(
                        f"mc_{y:04}_{m:02}_{d:02}_*.root",
                        root_dir=f"{path}/{y:04}/{m:02}",
                    )

                    if not len(candidates):
                        continue
                    elif len(candidates) == 1:
                        full_file_paths.append(f"{path}/{y:04}/{m:02}/{candidates[0]}")
                    elif len(candidates) > 1:
                        query = "(0) ALL FILES BELOW\n"
                        for i, c in enumerate(candidates, 1):
                            query += f"({i}) {c}\n"
                        print(query, flush=True)
                        which = int(
                            input(f"More than one file found, which to choose? ")
                        )

                        if which:
                            full_file_paths.append(
                                f"{path}/{y:04}/{m:02}/{candidates[which-1]}"
                            )
                        else:
                            for c in candidates:
                                full_file_paths.append(f"{path}/{y:04}/{m:02}/{c}")

                    break
                else:
                    #self.logger.error(
                    #    f"I cannot find the monit file for {y:04}-{m:02}-{d:02} !!!"
                    #)
                    raise FileNotFoundError(
                        f"mc_{y:04}_{m:02}_{d:02}_*h*.root not found in any data path you've specified"
                    )

        #self.logger.info(f"received {len(full_file_paths)} file(s) as input")

        """
        opening individual files is faster than concatenate, iterate etc.,
        because we dont immediately load everything into memory at once
        """
        self.__streams = [
            uproot.open(f"{file}:{starting_branch}") for file in full_file_paths
        ]

        """these keys surely generalize to the entire dataset..."""
        temp, self._keys = self.__streams[0].keys(), {}
        temp.sort(key=lambda x: x.count("/"))

        for key in temp:
            try:
                branch, name = key.split("/")
                self._keys[branch][name.split(".")[-1].replace("[3]", "")] = key

            except (ValueError, KeyError):
                if key in ["fLsId", "fTime", "fCDASTime"]:
                    self._keys[key] = key
                else:
                    self._keys[key] = {}

        #self.logger.info(f"View monit keys with self.keys()")

    def __getitem__(self, item) -> typing.Union[dict, str]:
        return self.get_key(item) or self._keys[item]

    def __call__(self, path, station: int = -1) -> np.ndarray:
        """Fetching multiple stations is discouraged due to runtime performance"""
        result = []

        easy_path = self.get_key(path)
        full_path = path if easy_path is None else easy_path

        for stream in self.__streams:
            data = stream[full_path].array()

            if station != -1:
                station_list = stream["fLsId"].array()
                station_mask = station_list == station
            else:
                station_mask = [True for _ in range(len(data))]

            [result.append(x) for x in data[station_mask]]

        maybe_station = f" and station #{station}" if station != -1 else ""
        #self.logger.info(f"found {len(result)} entries for key {path}{maybe_station}")
        return np.array(result)

    def get_key(self, key) -> typing.Union[None, str]:
        for branch in ["fMonitoring", "fRawMonitoring", "fCalibration"]:
            try:
                return self._keys[branch][key]
            except KeyError:
                continue

    def keys(self) -> typing.NoReturn:
        print(json.dumps(self._keys, indent=2))
