import typing
from ...binaries import np
import uproot

class Monit():

    monit_path = '/cr/auger02/Prod/monit/Sd/'

    def __init__(self, year : int, month : int, day : int, starting_branch : str = "SDMonCal/SDMonCalBranch", monit_file : str = None, IDs : np.ndarray = None) -> None :

        if monit_file is None:
            self.monit_file = f"{self.monit_path}/{year}/{month}/mc_{year}_{month}_{day}_00h00.root"
        else:
            self.monit_file = monit_file
        
        self.starting_branch = starting_branch
        self.__stream = uproot.open(f"{self.monit_file}:{starting_branch}")
        self.IDs = self.__stream['fLsId'].array() if IDs is None else IDs


    def __getattr__(self, name: str) -> typing.Union[typing.Callable, set] :

        hits = [True if name in key else False for key in self.__stream.keys()]

        if sum(hits) == 0:
            print(self)
            raise KeyError(f"'{name}' not found in online monitoring!")
        elif sum(hits) != 1:

            first_key = self.__stream.keys()[np.argmax(hits)].split('/')[-1]
            print(f"Multiple entries found for {name}, returning first occurence: {first_key}")
            return Monit(..., ..., ..., 'SDMonCal/' + first_key, self.monit_file, self.IDs)

            # raise ValueError("Fetching of entire branches is not encouraged!")
        
        else:
            if name.lower() in ['id', 'flsid', 'ids']: return set(self.IDs)
        
        return lambda x: np.array(self.__stream[self.__stream.keys()[np.argmax(hits)]].array())[np.array(np.argwhere(self.IDs == x)).flatten()]
    
    
    def __str__(self) -> str : 

        s  = f"File: {self.monit_file}\n"
        s += f"on branch: {self.starting_branch}\n"
        s += "\n".join(self.format_keys(self.__stream.keys()))

        return s
    
     
    def format_keys(self, keys : list) -> list :

        for i, key in enumerate(keys):
            k = key.split('/') 
            n_indent = len(k)
            indent = "  " * n_indent
            mod = "-:" if n_indent != 0 else ""
            key = k[-1].split('.')[-1]
            keys[i] = f"{indent}{mod} {key}"

        return keys