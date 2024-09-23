#!/usr/bin/python3

import os, sys
import subprocess
proc_no = int(sys.argv[1])

def determine_source_file(proc_no) -> list[str] :
    source_dir = "/lsdf/auger/corsika/prague/EPOS_LHC/photon/16.5_17/"
    files = [file for file in os.listdir(source_dir) if '.' not in file]

    return source_dir + files[proc_no], files[proc_no]

file, name = determine_source_file(proc_no)

NUM_RETHROWS = 1
DETECTORSEED = proc_no
PHYSICSSEED = proc_no + 1

for _ in range(NUM_RETHROWS):
    target_file = f"/cr/work/filip/SSDTriggerEfficiency/run01/{name.split('.')[0]}_{DETECTORSEED:06}.root"
    target_bootstrap = f'/cr/work/filip/Simulations/Bootstraps/bootstrap_{proc_no}_{DETECTORSEED}.xml'

    # Prepare bootstrap
    replacements = {
        '@INPUTFILE@' : file,
        '@DETECTORSEED@' : f"{DETECTORSEED:06}",
        '@PHYSICSSEED@' : f"{PHYSICSSEED:06}",
        '@NPARTICLES@' : '30000',
        # '@NPARTICLES@' : '1000',
        '@OUTPUTFILE@' : target_file,
        '@PATTERNPATH@' : '*.part' if file.endswith(".part") else "*",
        '@GROUNDDATA@' : '(1).part' if file.endswith(".part") else "(1)",
    }

    DETECTORSEED += 1
    PHYSICSSEED += 1

    with open(target_bootstrap, 'w') as target:
        with open('../SdSimulationReconstructionUpgrade/bootstrap.xml', 'r') as source:
            for line in source.readlines():
                try:
                    target.write(replacements[line.strip()] + "\n")
                except KeyError: target.write(line)

    subprocess.call(["./run_simulation.sh", target_bootstrap, target_file])