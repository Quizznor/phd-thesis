#!/usr/bin/python3

import os, sys
import subprocess

# TODO: intelligently grab source file

# DETECTORSEED = 0
# PHYSICSSEED = 1

source_file = "/lsdf/auger/corsika/napoli/QGSJET-II.04/proton/19.5_20/DAT100012.lst"
source_name, proc_no = source_file.split('/')[-1], int(sys.argv[1])

DETECTORSEED = proc_no
PHYSICSSEED = proc_no + 1

target_file = f"/cr/work/filip/SSDTriggerEfficiency/run01/{source_name.split('.')[0]}_{DETECTORSEED:06}.root"
target_bootstrap = f'/cr/work/filip/Simulations/Bootstraps/bootstrap_{proc_no}.xml'

offline_config_path = "/cr/data01/filip/offline/bd0e9e/install/share/auger-offline/config"
replace_config_dir = lambda x: x.replace("@CONFIGDIR@", offline_config_path)

# Prepare bootstrap
replacements = {
    '@INPUTFILE@' : source_file,
    '@PATTERNPATH@' : '*.lst',                      # TODO: will fail for some shit
    '@GROUNDDATA@' : '(1).lst',                     # TODO: will fail for some shit
    '@DETECTORSEED@' : f"{DETECTORSEED:06}",
    '@PHYSICSSEED@' : f"{PHYSICSSEED:06}",
    '@NPARTICLES@' : '1000',                    # DEFAULT: 300000
    '@OUTPUTFILE@' : target_file,
}

with open(target_bootstrap, 'w') as target:
    with open('../SdSimulationReconstruction/bootstrap.xml', 'r') as source:
        for line in source.readlines():
            try:
                target.write(replacements[line.strip()] + "\n")
            except KeyError:
                target.write(replace_config_dir(line))

subprocess.call(["./run_simulation.sh", target_bootstrap, target_file])