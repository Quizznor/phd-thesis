#!/usr/bin/python3

import os, sys
import subprocess

CRWORK = "/cr/work/filip/Simulations"

def prepare_bootstrap(arguments, seed) -> bool:

    is_valid = (lambda s: s.startswith("DAT")
                and not s.endswith(".long")
                and not s.endswith(".lst")
                and not s.endswith(".gz"))

    name, primary, energy, rethrows, n_particles, proc_no = arguments
    
    hadron_prefix = "prague" if energy < "18.5_19" else "napoli"
    photon_prefix = "prague"
    data_dirs = {
        "photon": f"/lsdf/auger/corsika/{photon_prefix}/EPOS_LHC/photon/{energy}/",
        "proton": f"/lsdf/auger/corsika/{hadron_prefix}/EPOS_LHC/proton/{energy}/",
    }


    source_dir = data_dirs[primary]
    files = list(filter(is_valid, os.listdir(source_dir)))
    target_dir = f"/cr/work/filip/Simulations/{name}"
    source_file = files[int(proc_no)]
    target_file = f"{target_dir}/out/{primary}/{energy}/{source_file.split('.')[0]}_{seed}.root"
    target_bootstrap = f"{target_dir}/sim/bootstrap_{seed:06}.xml"

    replacements = {
        "@INPUTFILE@": f"{source_dir}/{source_file}",
        "@NPARTICLES@": n_particles,
        "@OUTPUTFILE@": target_file,
        "@DETECTORSEED@": f"{DSEED:06}",
        "@PHYSICSSEED@": f"{PSEED:06}",
        "@PATTERNPATH@": "*.part" if source_file.endswith(".part") else "*",
        "@GROUNDDATA@": "(1).part" if source_file.endswith(".part") else "(1)",
    }

    with open(target_bootstrap, "w") as target:
        with open(f"{CRWORK}/{name}/src/bootstrap.xml", "r") as source:
            for line in source.readlines():
                try:
                    target.write(replacements[line.strip()] + "\n")
                except KeyError:
                    target.write(line)
                    
    return target_bootstrap


if __name__ == "__main__":

    arguments = (NAME, PRIMARY, ENERGY, RETHROWS, N_PARTICLES, PROC) = sys.argv[1:]
    os.chdir(f"{CRWORK}/{NAME}/src/")

    for DSEED, PSEED in zip(range(0, int(RETHROWS)), range(1, int(RETHROWS)+1)):
        DSEED, PSEED = f"{DSEED:06}", f"{PSEED:06}"    

        if target_bootstrap := prepare_bootstrap(arguments, DSEED):
            subprocess.run([f"{CRWORK}/{NAME}/run.sh", target_bootstrap])
