from ...binaries.binary_tools import ProgressBar
from ... import CONSTANTS
from pathlib import Path
import subprocess
import json
import stat
import os

condor_default_dict = {
            'max_idle': "150",              # don't insert jobs if max_idle is more than this
            'request_memory': "1G",         # put job on hold if RAM exceeds request_memory
            'max_materialize': "150",       # number of total active/running/idle jobs on condor

            # cpu, gpu etc.
        }

python_default_dict = {
    # TODO
}

class Simulation():

    CROFFLINE = f'/cr/data01/{CONSTANTS.USERNAME}/offline/install'
    CRSRC = f'/cr/users/{CONSTANTS.USERNAME}/bin/utils/auger'
    CRWORK = f'/cr/work/{CONSTANTS.USERNAME}/Simulations'

    def __init__(self, name: str, offline: str, src: str, primary: str, energy: str, model: str, **kwargs: dict):

        # make file system
        self.path = Path(f"{self.CRWORK}/{name}")
        self.path.mkdir(parents=True, exist_ok=True)
        self.offline_src = f"{self.CROFFLINE}/{offline}/set_offline_env.sh"

        for dir in ['src', 'sim', 'log']:
            Path(self.path / dir).mkdir(parents=True, exist_ok=True)

        self.target_path = Path(self.path / "dat" / model / primary / energy)
        self.target_path.mkdir(parents=True, exist_ok=True)
        
        # set condor/python kwargs
        self.condor_kwargs, self.python_kwargs = self._get_simulation_kwargs(primary, energy, model, kwargs)

        # make run.sub file
        sub_path = self.path / "run.sub"
        with sub_path.open("w", encoding="utf-8") as sub:
            sub.write(CONSTANTS.SIM_HEADER)
            sub.write("\n\n")
            sub.write(CONSTANTS.SIM_REQS)
            sub.write("\n\n")
            for key, value in self.condor_kwargs.items():
                sub.write(f"{key: <24}= {value}\n")
            sub.write(f'\nqueue {len([1,2])}')

        # make run.sh file
        sh_path = self.path / "run.sh"
        with sh_path.open("w", encoding="utf-8") as sh:
            sh.write("#!/bin/bash\n")
            sh.write(f"\nsource {self.offline_src}\n")
            sh.write("./userAugerOffline --bootstrap $1\n")
            sh.write("rm -rf *.root *.dat $1")
        sh_path.chmod(sh_path.stat().st_mode | stat.S_IEXEC)

        # make run.py file
        py_path = self.path / "run.py"
        with py_path.open("w", encoding="utf-8") as py:
            py.write("#!/usr/bin/python3\n\n")
            py.write("import os, subprocess\n\n")
            py.write("is_corsika_file = (lambda s: s.startswith('DAT')\n")
            py.write("\tand not s.endswith('.long')\n")
            py.write("\tand not s.endswith('.lst')\n")
            py.write("\tand not s.endswith('.gz'))\n\n")
        py_path.chmod(py_path.stat().st_mode | stat.S_IEXEC)
        
        # set up source
        subprocess.run("; ".join([
            f"source {self.offline_src}",
            f"cd {src}",
            f"cp Makefile.in Makefile",
            f"make",
            f"mv *.xml userAugerOffline {self.path}/src",
            f"rm -rf Makefile *.o Make-depend"
        ]), shell=True, executable='/bin/bash', stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


    def _get_simulation_kwargs(self, primary: str, energy: str, model: str, kwargs: dict) -> dict:

            self.model = model
            self.energy = energy
            self.primary = primary

            condor_kwargs = self._get_condor_kwargs(kwargs)

            # get source path of simulation files
            target_path = Path('/lsdf/auger/corsika')
            min_energy, _ = [float(x) for x in energy.split('_')]
            if min_energy <= 18.5:
                library = "prague"
            else: library = "napoli"
            target_path /= f"{library}/{model.upper()}/{primary.lower()}/{energy}"

            if not os.path.isdir(target_path):
                raise LookupError(f"Data dir not found for keys {model}, {primary}, {energy}")       
            
            # files = list(filter(is_corsika_file, os.listdir(target_path)))

            # raise NotImplementedError

            # todo fix python kwargs
            # create run.py script            

            return condor_kwargs, {}

            # QUEUE, args = kwargs.get("QUEUE", self.QUEUE), []
            # for arg in ["PRIMARY", "ENERGY", "RETHROWS", "NPARTICLES"]:
            #     args.append(kwargs.get(arg, getattr(self, arg)))
            #     kwargs[arg] = kwargs.get(arg, getattr(self, arg))

            # kwargs['max_idle'] = kwargs.get("max_idle", self.MAX_IDLE)
            # kwargs['max_materialize'] = kwargs.get("max_materialize", self.MAX_MATERIALIZE)
            # kwargs['request_memory'] = kwargs.get("request_memory", self.MEMORY)

            # # num_cores, etc...

            # kwargs['JobBatchName'] = f"{self.path.name}_{args[0]}_{args[1]}"
            # kwargs['arguments'] = kwargs.get("arguments", f'"{self.path.name} {" ".join(args)} $(Process)"')
            


            # return kwargs, QUEUE


    def _get_condor_kwargs(self, kwargs) -> dict:
            
        condor_dict = condor_default_dict
        job_batch_name = f"{self.path.name}_{self.model}_{self.primary}_{self.energy}"
        condor_dict['JobBatchName'] = job_batch_name
        condor_dict['error'] = str(self.path / f"log/{job_batch_name}-$(Process).err")
        condor_dict['output'] = str(self.path / f"log/{job_batch_name}-$(Process).out")
        condor_dict['log'] = str(self.path / f"log/{job_batch_name}-$(Process).log")

        # overwrite defaults in condor dict
        for key, val in kwargs.items():
            if condor_dict.get(key, None) is not None:
                condor_dict[key] = val

        return condor_dict


    def process(self, executable: str) -> None:

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


    # @staticmethod
    # def tree_view(path: str) -> None:

    #     primaries = os.listdir(path)
    #     energies = set()
    #     [[energies.add(e) for e in os.listdir(path/p)] for p in primaries]

    #     for p in primaries:
    #         print(f"  {p}")
    #         for dir in energies:
    #             print(f"    {dir}: {len(os.listdir(path/p/dir)): >5} files")


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
