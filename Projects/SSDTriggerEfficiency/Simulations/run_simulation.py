#!/usr/bin/python3

import os, sys
import subprocess


def determine_source_file(arguments) -> list[str]:

    is_valid = (
        lambda s: s.startswith("DAT")
        and not s.endswith(".long")
        and not s.endswith(".lst")
        and not s.endswith(".gz")
    )

    primary, energy, proc_no = arguments
    proc_no = int(proc_no)

    hadron_prefix = "prague" if energy < "18.5_19" else "napoli"
    data_dirs = {
        "photon": f"/lsdf/auger/corsika/prague/EPOS_LHC/photon/{energy}/",
        "proton": f"/lsdf/auger/corsika/{hadron_prefix}/EPOS_LHC/proton/{energy}/",
    }

    try:
        source_dir = data_dirs[primary]
        files = list(filter(is_valid, os.listdir(source_dir)))
        return proc_no, source_dir + files[proc_no], files[proc_no]

    except KeyError:
        sys.exit("No valid primary")


proc_no, file, name = determine_source_file(sys.argv[1:])

NUM_RETHROWS = 1
DSEED, PSEED = 0, 1

for _ in range(NUM_RETHROWS):
    target_file = f"/cr/work/filip/SSDTriggerEfficiency/run01/{name.split('.')[0]}_{DSEED:06}.root"
    target_bootstrap = (
        f"/cr/work/filip/Simulations/Bootstraps/bootstrap_{proc_no}_{DSEED}.xml"
    )

    # Prepare bootstrap
    replacements = {
        "@INPUTFILE@": file,
        "@NPARTICLES@": "30000",
        "@OUTPUTFILE@": target_file,
        "@DETECTORSEED@": f"{DSEED:06}",
        "@PHYSICSSEED@": f"{PSEED:06}",
        "@PATTERNPATH@": "*.part" if file.endswith(".part") else "*",
        "@GROUNDDATA@": "(1).part" if file.endswith(".part") else "(1)",
    }

    DSEED += 1
    PSEED += 1

    with open(target_bootstrap, "w") as target:
        # with open('../SdSimulationReconstructionUpgrade/bootstrap.xml', 'r') as source:
        with open("bootstrap.xml", "r") as source:
            for line in source.readlines():
                try:
                    target.write(replacements[line.strip()] + "\n")
                except KeyError:
                    target.write(line)

    subprocess.call(["./run_simulation.sh", target_bootstrap, target_file])
