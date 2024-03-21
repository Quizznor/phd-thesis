from ...binaries import np
from typing import Union
import bz2
import os

tuple_or_array = Union[tuple, np.ndarray]

class UubRandom():

    dir = '/cr/data02/AugerPrime/UUB/UubRandoms/'
    dates = {
        'NuriaJr' : '2022_11',
        'Constanza' : '2023_03',
    }
    fmt = {
        'wcd' : np.dtype([('timestamp', 'I'), 
                        ('t1_latch_bin', 'h'), 
                        ('vem_peak', ('e', 3)), 
                        ('baseline', ('H', 3)), 
                        ('traces', ('h', (3, 2048)))]),
        'ssd' : np.dtype([('timestamp', 'I'), 
                        ('t1_latch_bin', 'h'), 
                        ('mip_peak', 'e'), 
                        ('baseline', 'H'), 
                        ('trace', ('h', 2048))])
    }



    def __init__(self, station : str, detectors : str = 'all') -> None :

        try:
            date = self.dates[station]
        except KeyError:
            raise NameError('Station does not exist!')
        
        self.path = self.dir + '/'.join([date, station])

        match detectors:
            case 'all':
                self.extensions = ['wcd', 'ssd']
            case 'wcd':
                self.extensions = ['wcd']
            case 'ssd':
                self.extensions = ['ssd']

    def __iter__(self) -> 'UubRandom' :
        self.__index = 0
        return self
    
    def __next__(self) -> tuple_or_array :

        if self.__index == self.__len__(): raise StopIteration
        data = self[self.__index]
        self.__index += 1
        return data

    def __len__(self) -> int :
        basename = [f.split('.')[0] for f in os.listdir(self.path)]
        return len(np.unique(basename))

    def __getitem__(self, index : int) -> tuple_or_array :
        return self.read(index)

    def read(self, index : int) -> np.ndarray :

        results = []
        for ext in self.extensions:
            file_path = f"{self.path}/randoms{index:04}.{ext}.bz2"
            buffer = bz2.BZ2File(file_path).read()
            results.append(np.frombuffer(buffer, self.fmt[ext]))
            
        return results if len(results) != 1 else results[0]
    
def filter_and_downsample(*traces : np.ndarray, random_phase : int = 1) -> list :
    
    filtered_and_downsampled_traces = []

    # see Framework/SDetector/UUBDownsampleFilter.h in Offline main branch for more information
    kFirCoefficients = np.array([ 5, 0, 12, 22, 0, -61, -96, 0, 256, 551, 681, 551, 256, 0, -96, -61, 0, 22, 12, 0, 5 ])
    buffer_length = int(0.5 * len(kFirCoefficients))
    kFirNormalizationBitShift = 11
    kADCSaturation = 4095
    kFirLen = len(kFirCoefficients)

    for pmt in traces:
        temp = np.zeros(len(pmt) + len(kFirCoefficients), dtype=int)
        temp[0 : buffer_length] = pmt[:: -1][-buffer_length - 1 : -1]
        temp[-buffer_length - 1: -1] = pmt[:: -1][0 : buffer_length]
        temp[buffer_length : -buffer_length - 1] = pmt

        temp_shifted = np.array([temp[k:k+len(pmt)] for k in range(kFirLen)])
        outer_product = temp_shifted * kFirCoefficients[:, np.newaxis]

        trace = np.sum(outer_product, axis=0)
        trace = np.clip(np.right_shift(trace, kFirNormalizationBitShift), 0, kADCSaturation)

        trace = trace[random_phase::3]
        filtered_and_downsampled_traces.append(np.array(trace, dtype='u2'))

    return filtered_and_downsampled_traces