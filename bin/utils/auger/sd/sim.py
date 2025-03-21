from ...binaries.binary_tools import ProgressBar
from ... import CONSTANTS, create_stream_logger
from tabulate import tabulate
from typing import Iterator
from ...binaries import np
from pathlib import Path
import subprocess
import json
import stat
import os

condor_default_dict = {
            'executable': "./run.py",       # job script for HTCondor to execute on nodes
            'arguments': "$(Process)",      # arguments (sep. by whitespace) for job script
            'max_idle': "150",              # don't insert jobs if n_jobs_idle is more than this
            'request_memory': "1G",         # put job on hold if RAM exceeds request_memory
            'max_materialize': "150",       # number of total active/running/idle jobs on condor
            'should_transfer_files': "YES"  # needed to transfer output root file to /cr/work

            # cpu, gpu etc.
        }

python_default_dict = {
    'rethrows': 1,                          # how many times shower is simulated (with different seed)
    'n_particles': 30_000,                  # decrease number for quick and dirty test simulation
}

class Simulation():

    CROFFLINE = f'/cr/data01/{CONSTANTS.USERNAME}/offline/install'
    CRWORK = f'/cr/work/{CONSTANTS.USERNAME}/Simulations'

    def __init__(self, name: str = None, 
                 offline: str = None, 
                 src: str = None, 
                 primary: str = None, 
                 energy: str = None, 
                 model: str = None, 
                 **kwargs: dict):
        
        self.logger = create_stream_logger('sim_logger')
        self.path = Path(f"{self.CRWORK}/{name}")

        # fake __init__
        if offline is None \
            or src is None \
            or primary is None \
            or energy is None \
            or model is None:
            
            self.logger.info("dummy initialization!")
            self.fake_init = True
            return
        else:
            self.fake_init = False
        
        # make file system
        self.path.mkdir(parents=True, exist_ok=True)
        self.offline_src = f"{self.CROFFLINE}/{offline}/set_offline_env.sh"

        for dir in ['src', 'sim']:
            Path(self.path / dir).mkdir(parents=True, exist_ok=True)

        self.target_path = Path(self.path / "dat" / model / primary / energy)
        self.target_path.mkdir(parents=True, exist_ok=True)

        log_path = Path(self.path / "log" / model / primary / energy)
        log_path.mkdir(parents=True, exist_ok=True)

        self.logger.info("filesystem established successfully")
        
        
        # set condor/python kwargs
        self.condor_kwargs, self.python_kwargs, queue = self._get_simulation_kwargs(primary, energy, model, kwargs)
        self.logger.info(f"Corsika dir found, {queue} files available")

        self.work_path = self.path / f"work/{model}_{primary}_{energy}"
        self.work_path.mkdir(parents=True, exist_ok=True)

        # make send file
        send_path = self.path / "send.sh"
        with send_path.open("w", encoding="utf-8") as send:
            send.write("#!/bin/sh\n")
            send.write("cd work/$1_$2_$3\n")
            send.write(f"condor_submit condor.sub")
        send_path.chmod(send_path.stat().st_mode | stat.S_IEXEC)

        # make run.sub file
        sub_path = self.work_path / "condor.sub"
        with sub_path.open("w", encoding="utf-8") as sub:
            sub.write(CONSTANTS.WORD.SIM_HEADER)
            sub.write("\n\n")
            sub.write(CONSTANTS.WORD.SIM_REQS)
            sub.write("\n\n")
            for key, value in self.condor_kwargs.items():
                sub.write(f"{key: <24}= {value}\n")
            sub.write(f'\nqueue {queue}')

        # make run.sh file
        sh_path = self.work_path / "run.sh"
        with sh_path.open("w", encoding="utf-8") as sh:
            sh.write("#!/bin/bash\n")
            sh.write(f"\nsource {self.offline_src}\n")
            sh.write(f"cd {self.path}/src/ && ./userAugerOffline --bootstrap $1\n")
            sh.write("rm -rf *.root *.dat $1")
        sh_path.chmod(sh_path.stat().st_mode | stat.S_IEXEC)

        # make run.py file
        py_path = self.work_path / "run.py"
        with py_path.open("w", encoding="utf-8") as py:
            py.write(CONSTANTS.WORD.RUN_PY_HEADER)
            py.write(f"NAME = \"{self.python_kwargs['name']}\"\n")
            py.write(f"SRC = \"{self.python_kwargs['src']}\"\n")
            py.write(f"OUT = \"{self.python_kwargs['out']}\"\n")
            py.write(f"RETHROWS = {self.python_kwargs['rethrows']}\n")
            py.write(f"n = {self.python_kwargs['n_particles']}\n")
            py.write(CONSTANTS.WORD.RUN_PY_FOOTER)
        py_path.chmod(py_path.stat().st_mode | stat.S_IEXEC)

        self.logger.info("scripts have been written")
        
        # set up source
        proc = subprocess.run("; ".join([
            f"source {self.offline_src}",
            f"cd {src}",
            f"cp Makefile.in Makefile",
            f"make",
            f"mv *.xml userAugerOffline {self.path}/src",
            f"rm -rf Makefile *.o Make-depend"
        ]), shell=True, executable='/bin/bash', stdout=subprocess.DEVNULL)

        self.logger.info("source compiled, we're done!")
        self.status(full_status=True)


    def _get_simulation_kwargs(self, primary: str, energy: str, model: str, kwargs: dict) -> dict:

            is_sim_file = (lambda s: s.startswith("DAT")
                           and not s.endswith(".long")
                           and not s.endswith(".lst")
                           and not s.endswith(".gz"))

            condor_kwargs = self._get_condor_kwargs(primary, energy, model, kwargs)

            # get source path of simulation files
            target_path = Path('/lsdf/auger/corsika')
            min_energy, _ = [float(x) for x in energy.split('_')]
            if min_energy <= 18.5:
                library = "prague"
            else: library = "napoli"
            target_path /= f"{library}/{model.upper()}/{primary.lower()}/{energy}"

            if not os.path.isdir(target_path):
                raise LookupError(f"Data dir not found for keys {model}, {primary}, {energy}")
            
            python_kwargs = python_default_dict
            python_kwargs["name"] = str(self.path)
            python_kwargs["src"] = str(target_path)
            python_kwargs["out"] = f"dat/{model}/{primary}/{energy}"

            for key, val in kwargs.items():
                if python_default_dict.get(key, None) is not None:
                    python_kwargs[key] = val           

            return condor_kwargs, python_kwargs, len(list(filter(is_sim_file, os.listdir(target_path))))
    

    def _get_condor_kwargs(self, primary, energy, model, kwargs) -> dict:

        condor_dict = condor_default_dict
        condor_dict['JobBatchName'] = f"{self.path.name}_{model}_{primary}_{energy}"
        condor_dict['error'] = str(self.path / f"log/{model}/{primary}/{energy}/{self.path.name}-$(Process).err")
        condor_dict['output'] = str(self.path / f"log/{model}/{primary}/{energy}/{self.path.name}-$(Process).out")
        condor_dict['log'] = str(self.path / f"log/{model}/{primary}/{energy}/{self.path.name}-$(Process).log")

        # overwrite defaults in condor dict
        for key, val in kwargs.items():
            if condor_dict.get(key, None) is not None:
                condor_dict[key] = val

        return condor_dict


    def cleanup(self, log: bool=True, out: bool=False, dat: bool=False):

        raise NotImplementedError

        if log: os.system(f"rm -rf {self.path / 'log/*'}")

        # TODO
        if out: raise NotImplementedError; os.system(f"rm -rf {self.path / 'out/*'}")
        if dat: raise NotImplementedError; os.system(f"rm -rf {self.path / 'dat/*'}")


    def run(self, proc_no: int) -> int:
        return subprocess.run([f"cd {self.work_path}; ./run.py {proc_no}"], shell=True)


    def __str__(self) -> str:
        return json.dumps(self.kwargs, indent=2)


    def status(self, full_status: bool = False) -> None:

        print("")
        print("*****************************")
        print("* OFFLINE SIMULATION STATUS *")
        print("*****************************")

        if full_status:
            for _dict, handle in zip([self.python_kwargs, self.condor_kwargs], ["python", "condor"]):
                for key, val in _dict.items():
                    print(f"{handle}: {key} = {val}")
                print("")

        energy_bins = ["15_15.5", "15.5_16", 
                       "16_16.5", "16.5_17", 
                       "17_17.5", "17.5_18",
                       "18_18.5", "18.5_19",
                       "19_19.5", "19.5_20",
                       "20_20.2"]
            
        for model in os.listdir(self.path / "dat"):
            print(f"** {model} (root / other) **")
            table = []
            for primary in os.listdir(self.path / f"dat/{model}"):
                row = [primary]
                for _bin in energy_bins:
                    try:
                        all_files = os.listdir(self.path / f"dat/{model}/{primary}/{_bin}")
                        root = sum([f.endswith(".root") for f in all_files])
                        row.append(f"{root} / {len(all_files) - root}")
                    except FileNotFoundError:
                        row.append("")
                table.append(row)

        print(tabulate(table, headers=energy_bins))


    @classmethod
    def help(cls) -> None:
        
        print('"""')
        print("Wrapper around Offline Simulation software")
        print()
        print("\tRequired arguments:")
        print("\t\tname                            -- directory name in /cr/work/filip/Simulations")
        print("\t\toffline                         -- offline build in /cr/data01/filip/Offline")
        print("\t\tsrc                             -- source directory with bootstrap, userAugerOffline")
        print("\t\tmodel                           -- which hadronic interaction model to use")
        print("\t\tprimary                         -- which primary particle to simulate")
        print("\t\tenergy                          -- energy (bin) of the primary particle")
        print()
        print("\tKeyword arguments:")
        print("\t\trethrows             == 1       -- how many rethrows for each shower (default: 1)")
        print("\t\trequest_memory       == 1G      -- how much RAM to request for each job")
        print("\t\tmax_idle             == 150     -- max jobs to put into idle queue")
        print("\t\tn_particles          == 30000   -- for quick/dirty simulation tests")
        print("\t\tmax_materialize      == 150     -- max jobs to put into queue")
        print('"""')


class SimData():

    def __init__(self, name: str, model: str, primary: str) -> None:

        self.name = name
        self.model = model
        self.primary = primary
        path = Path(f'/cr/work/{CONSTANTS.USERNAME}/Simulations/{name}/dat/{model}/{primary}')

        self.files = []
        for energy in os.listdir(path):
            files = os.listdir(path / energy)
            is_candidate = lambda p: p.endswith("csv")
            self.files += [path / f"{energy}/{f}" for f in filter(is_candidate, files)]

    
    def __len__(self) -> int:
        return len(self.files)

    
    def __iter__(self) -> Iterator[np.ndarray]:

        iteration_index = 0
        while iteration_index < len(self):
            yield Shower(np.loadtxt(self.files[iteration_index]))
            iteration_index += 1
          
        return StopIteration


class Shower():

    def __init__(self, event_data: list[np.ndarray]) -> None:

        energy = set(event_data[:, 0])
        zenith = set(event_data[:, 1])
        assert len(energy) == len(zenith) == 1, "Malformed shower data"

        self.energy = energy.pop()
        self.zenith = zenith.pop()
        self.stations = []

        for station_data in np.split(event_data[:, 2:], len(event_data) // 4):
            self.stations.append(Station(station_data))

    
    def __repr__(self) -> str:
        return f"Shower w/ log10(E/eV) = {np.log10(self.energy):.2f} and {len(self.stations)} stations"

    
    def __getitem__(self, idx: int) -> "Station":
        return self.stations[idx]

    
    def trigger(self, fctn: callable) -> list[bool]:
        stations, triggers = [], []
        for station in self:
            triggers.append(station.trigger(fctn))
            stations.append(station.id)

        return stations, triggers
    

class Station():

    def __init__(self, station_data) -> None:
        ids = set(station_data[:, 0])
        spd = set(station_data[:, 1])
        assert len(ids) == len(spd) == 1, "Malformed station data"

        self.id = int(ids.pop())
        self.spd = spd.pop()

        self.wcd = station_data[:-1, 2:]
        self.ssd = station_data[-1, 2:]

    
    def __repr__(self) -> str:
        return f"Station {self.id} @ {self.spd}m from core"


    def trigger(self, fctn: callable) -> bool:
        return fctn(self.wcd, self.ssd)