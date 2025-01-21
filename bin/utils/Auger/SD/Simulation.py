from ... import CONSTANTS
from pathlib import Path
import stat
import os

class Simulation():

    CROFFLINE = 'cr/data01/filip/offline/install'
    CRWORK = '/cr/work/filip/Simulations'

    QUEUE = 1
    RETHROWS = 1
    MEMORY = "1G"
    MAX_IDLE = 150
    PRIMARY = 'proton'
    ENERGY = '16.5_17'
    MAX_MATERIALIZE = 150

    def __init__(self, name: str, offline: str, **kwargs: dict):

        # make file system
        self.path = Path(f"{self.CRWORK}/{name}")
        self.path.mkdir(parents=True, exist_ok=True)
        self.kwargs, QUEUE = self.get_simulation_kwargs(kwargs)

        for dir in ['src', 'out', 'sim', 'log']:
            Path(self.path / dir).mkdir(parents=True, exist_ok=True)

        # make run .sub file
        sub_path = self.path / "run.sub"
        with sub_path.open("w", encoding="utf-8") as sub:
            sub.write(CONSTANTS.SIM_HEADER)
            sub.write("\n\n")
            sub.write(CONSTANTS.SIM_REQS)
            sub.write("\n\n")
            for key, value in kwargs.items():
                if key.isupper(): continue
                sub.write(f"{key: <24}= {value}\n")

            sub.write(f'\nqueue {QUEUE}')

        # make run  .sh file
        sh_path = self.path / "run.sh"
        with sh_path.open("w", encoding="utf-8") as sh:
            sh.write("#/bin/bash\n")
            sh.write(f"\nsource {self.CROFFLINE}/{offline}/set_offline_env.sh\n")
            sh.write("./src/userAugerOffline --bootstrap $2\n")
            sh.write("rm -rf *.root src/*.root *.dat src/*.dat $2")

        sh_path.chmod(sh_path.stat().st_mode | stat.S_IEXEC)

        # make run  .py file
        os.system(f"cp /cr/users/filip/bin/utils/Auger/SD/run.py {self.path}")
        

    def get_simulation_kwargs(self, kwargs: dict) -> dict:

            PRIMARY = kwargs.get("PRIMARY", self.PRIMARY)
            ENERGY = kwargs.get("ENERGY", self.ENERGY)
            RETHROWS = kwargs.get("RETHROWS", self.RETHROWS)
            QUEUE = kwargs.get("QUEUE", self.QUEUE)

            kwargs['max_idle'] = kwargs.get("max_idle", self.MAX_IDLE)
            kwargs['max_materialize'] = kwargs.get("max_materialize", self.MAX_MATERIALIZE)
            kwargs['request_memory'] = kwargs.get("request_memory", self.MEMORY)

            # num_cores, etc...

            kwargs['arguments'] = kwargs.get("arguments", f'"{PRIMARY} {ENERGY} {RETHROWS} $(Process)"')
            kwargs['JobBatchName'] = f"{self.path.name}_{kwargs['PRIMARY']}_{kwargs['ENERGY']}"
            
            kwargs['executable'] = "./run.py"
            kwargs['error'] = self.path / f"log/{kwargs['JobBatchName']}-$(Process).err"
            kwargs['output'] = self.path / f"log/{kwargs['JobBatchName']}-$(Process).out"
            kwargs['log'] = self.path / f"log/{kwargs['JobBatchName']}-$(Process).log"

            return kwargs, QUEUE


    def cleanup(self, log: bool=True, out: bool=True, err: bool=True, data: bool=False):
        
        if log: os.system(f"rm -rf {self.path / 'log/*.log'}")
        if out: os.system(f"rm -rf {self.path / 'log/*.out'}")
        if err: os.system(f"rm -rf {self.path / 'log/*.err'}")

        if data: os.system(f"rm -rf {self.path / 'out/*'}")