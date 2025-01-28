from ...binaries import np, uncertainties
from ...binaries import tools
from typing import Union
import bz2
import os

# maybe make this asynchronous at some point?
TUPLE_OR_ARRAY = Union[tuple, np.ndarray]

class BackgroundStudy():

    # don't forget to @numba.jit the callable
    def __init__(self, fctn: callable, high_rate_warning: bool = False):

        self.time_passed, self.triggers = 0, 0
        RandomFiles = UubRandom(station = "Svenja", detectors = "ssd")
    
        self.trigger_examples = []
        for File in tools.ProgressBar(RandomFiles, newline=False):
            for event in File:

                calibrated = event['trace'] / event['mip_peak']
                self.time_passed += 2048 * 8.33e-9                  # add time

                if fctn(calibrated):                                # add trigger
                    self.trigger_examples.append(event)
                    self.triggers += 1
            
            if high_rate_warning:
                rate = uncertainties.ufloat(self.triggers, np.sqrt(self.triggers)) / self.time_passed
                if rate.n > 10: print(f"\n   !! HIGH Rate after {self.time_passed:.3f}s: ({rate}) Hz !!")

    def __str__(self) -> str:

        _str =  "*********************************  "
        _str += "\n*** UUB RANDOMS TRIGGER STUDY ***"
        _str += "\n*********************************"
        _str += f"\n\n  time passed:  {self.time_passed:.4f} s"
        _str +=   f"\n  triggers:     {self.triggers}"

        rate = uncertainties.ufloat(self.triggers, np.sqrt(self.triggers)) / self.time_passed
        _str += f"\n\n  rate:         {rate} Hz"

        return _str


class UubRandom:

    dir = "/cr/data02/AugerPrime/UUB/UubRandoms/"
    dates = {
        "NuriaJr": "2022_11",
        "Constanza": "2023_03",
        "Nadia": "2023_03",
        "Svenja": "2022_11",
    }
    fmt = {
        "wcd": np.dtype(
            [
                ("timestamp", "I"),
                ("t1_latch_bin", "h"),
                ("vem_peak", ("e", 3)),
                ("baseline", ("H", 3)),
                ("traces", ("h", (3, 2048))),
            ]
        ),
        "ssd": np.dtype(
            [
                ("timestamp", "I"),
                ("t1_latch_bin", "h"),
                ("mip_peak", "e"),
                ("baseline", "H"),
                ("trace", ("h", 2048)),
            ]
        ),
    }

    def __init__(self, station: str, detectors: str = "all") -> None:

        try:
            date = self.dates[station]
        except KeyError:
            raise NameError("Station does not exist!")

        self.path = self.dir + "/".join([date, station])

        match detectors:
            case "all":
                self.extensions = ["wcd", "ssd"]
            case "wcd":
                self.extensions = ["wcd"]
            case "ssd":
                self.extensions = ["ssd"]

    def __iter__(self) -> "UubRandom":
        self.__index = 0
        return self

    def __next__(self) -> TUPLE_OR_ARRAY:

        if self.__index == self.__len__():
            raise StopIteration
        data = self[self.__index]
        self.__index += 1
        return data

    def __len__(self) -> int:
        basename = [f.split(".")[0] for f in os.listdir(self.path)]
        return len(np.unique(basename))

    def __getitem__(self, index: int) -> TUPLE_OR_ARRAY:
        return self.read(index)

    def read(self, index: int) -> np.ndarray:

        results = []
        for ext in self.extensions:
            file_path = f"{self.path}/randoms{index:04}.{ext}.bz2"
            buffer = bz2.BZ2File(file_path).read()
            results.append(np.frombuffer(buffer, self.fmt[ext]))

        return results if len(results) != 1 else results[0]
