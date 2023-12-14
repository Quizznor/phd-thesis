import typing
from ... import create_stream_logger
monitoring_logger = create_stream_logger("Auger.SD.monit")
del create_stream_logger

from ...binaries import np
import uproot
monitoring_logger.info("import uproot")

class Monit():

    monit_path = '/cr/auger02/Prod/monit/Sd/'

    def __init__(self) -> None :

        self.__stream = uproot.open(f"{self.monit_path}/2023/11/mc_2023_11_18_00h00.root:SDMonCal")

    def __getattr__(self, name: str) -> typing.Any :

        hits = [1 if name in key else 0 for key in self.__stream.keys()]

        if sum(hits) == 0:
            print("Available keys are:")
            for key in self.__stream.keys(): print(key.split('/')[-1].split('[')[0])
            raise KeyError(f"'{name}' not found in online monitoring!")
        elif sum(hits) != 1:
            raise ValueError("Fetching of entire branches is not encouraged!")
        
        else:
            key = self.__stream.keys()[np.argmax(hits)]
            IDs = self.__stream['SDMonCalBranch/fLsId'].array()
            return lambda x: np.array(self.__stream[key].array())[np.array(np.argwhere(IDs == x)).flatten()]