from ...binaries.binary_tools import ProgressBar
from ... import CONSTANTS
from pathlib import Path
import subprocess
import json
import stat
import os

class Simulation():

    CROFFLINE = '/cr/data01/filip/offline/install'
    CRSRC = '/cr/users/filip/bin/utils/auger'
    CRWORK = '/cr/work/filip/Simulations'

    QUEUE = "1"
    RETHROWS = "1"
    MEMORY = "1G"
    MAX_IDLE = "150"
    PRIMARY = "proton"
    ENERGY = "16.5_17"
    NPARTICLES = "30000"
    MAX_MATERIALIZE = "150"

    def __init__(self, name: str, offline: str, src: str, **kwargs: dict):

        # make file system
        self.path = Path(f"{self.CRWORK}/{name}")
        self.path.mkdir(parents=True, exist_ok=True)
        self.kwargs, QUEUE = self.get_simulation_kwargs(kwargs)
        self.offline_src = f"{self.CROFFLINE}/{offline}/set_offline_env.sh"

        print(self.kwargs['PRIMARY'])

        for dir in ['src', 'sim', 'log']:
            Path(self.path / dir).mkdir(parents=True, exist_ok=True)
        for dir in ['out', 'dat']:
            Path(self.path / dir / self.kwargs['PRIMARY'] / self.kwargs['ENERGY']).mkdir(parents=True, exist_ok=True)

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
            sh.write("#!/bin/bash\n")
            sh.write(f"\nsource {self.offline_src}\n")
            sh.write("./userAugerOffline --bootstrap $1\n")
            sh.write("rm -rf *.root *.dat $1")

        sh_path.chmod(sh_path.stat().st_mode | stat.S_IEXEC)

        # make run  .py file
        os.system(f"cp {self.CRSRC}/sim/run.py {self.path}")
        
        # set up source
        os.system(f"{self.CRSRC}/sim/init.sh {self.offline_src} {src} {self.path}")


    def get_simulation_kwargs(self, kwargs: dict) -> dict:

            QUEUE, args = kwargs.get("QUEUE", self.QUEUE), []
            for arg in ["PRIMARY", "ENERGY", "RETHROWS", "NPARTICLES"]:
                args.append(kwargs.get(arg, getattr(self, arg)))
                kwargs[arg] = kwargs.get(arg, getattr(self, arg))

            kwargs['max_idle'] = kwargs.get("max_idle", self.MAX_IDLE)
            kwargs['max_materialize'] = kwargs.get("max_materialize", self.MAX_MATERIALIZE)
            kwargs['request_memory'] = kwargs.get("request_memory", self.MEMORY)

            # num_cores, etc...

            kwargs['JobBatchName'] = f"{self.path.name}_{args[0]}_{args[1]}"
            kwargs['arguments'] = kwargs.get("arguments", f'"{self.path.name} {" ".join(args)} $(Process)"')
            
            kwargs['executable'] = "./run.py"
            kwargs['error'] = str(self.path / f"log/{kwargs['JobBatchName']}-$(Process).err")
            kwargs['output'] = str(self.path / f"log/{kwargs['JobBatchName']}-$(Process).out")
            kwargs['log'] = str(self.path / f"log/{kwargs['JobBatchName']}-$(Process).log")

            return kwargs, QUEUE


    def process(self, executable: str= None) -> None:

        if executable is None:
            executable = f"{self.CRSRC}/ADST/AdstReader"

        data_dir = self.path / f"out/{self.kwargs['PRIMARY']}/{self.kwargs['ENERGY']}/"
        for file in ProgressBar(os.listdir(data_dir)):
            subprocess.run(" ".join([f". {self.offline_src} &&", executable, f"{data_dir}/{file}"]), 
                           shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


    def cleanup(self, log: bool=True, out: bool=False, dat: bool=False):
        
        if log: os.system(f"rm -rf {self.path / 'log/*'}")
        if out: os.system(f"rm -rf {self.path / 'out/*'}")
        if dat: os.system(f"rm -rf {self.path / 'dat/*'}")


    def run(self, proc_no: int) -> int:
        this_args = self.kwargs["arguments"].replace("$(Process)", str(proc_no))[1:-1]
        return subprocess.run([f"{self.path / 'run.py'}", *this_args.split(' ')])


    def __str__(self) -> str:
        return json.dumps(self.kwargs, indent=2)


    def status(self) -> None:

        print("*.root files present:")
        self.tree_view(self.path / "out")

        print("\n*.csv files present:")
        self.tree_view(self.path / "dat")

        print()
        print(self)


    @staticmethod
    def tree_view(path: str) -> None:

        primaries = os.listdir(path)
        energies = set()
        [[energies.add(e) for e in os.listdir(path/p)] for p in primaries]

        for p in primaries:
            print(f"  {p}")
            for dir in energies:
                print(f"    {dir}: {len(os.listdir(path/p/dir)): >5} files")


    @classmethod
    def help(cls) -> None:
        
        print('"""')
        print("Wrapper around Offline Simulation software")
        print()
        print("\tRequired arguments:")
        print("\t\tname                            -- directory name in /cr/work/filip/Simulations")
        print("\t\toffline                         -- offline build in /cr/data01/filip/Offline")
        print("\t\tsrc                             -- source directory with bootstrap, userAugerOffline")
        print()
        print("\tKeyword arguments:")
        print("\t\tQUEUE                == 1       -- how many showers to simulate (default: 1)")
        print("\t\tRETHROWS             == 1       -- how many rethrows for each shower (default: 1)")
        print("\t\tMEMORY               == 1G      -- how much RAM to request for each job")
        print("\t\tMAX_IDLE             == 150     -- max jobs to put into idle queue")
        print("\t\tPRIMARY              == proton  -- which primary particle to simulate")
        print("\t\tENERGY               == 16.5_17 -- energy (bin) of the primary particle")
        print("\t\tNPARTICLES           == 30000   -- for quick/dirty simulation tests")
        print("\t\tMAX_MATERIALIZE      == 150     -- max jobs to put into queue")
        print('"""')
