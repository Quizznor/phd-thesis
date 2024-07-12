#!/usr/bin/python3

import os, sys
import subprocess

# TODO: intelligently grab source file

DETECTORSEED = 0
PHYSICSSEED = 1

source_file = "/lsdf/auger/corsika/prague/QGSJET-II.04/proton/18_18.5/DAT010002"
source_name, proc_no = source_file.split('/')[-1], sys.argv[1]

target_file = f"/cr/work/filip/SSDTriggerEfficiency/run01/{source_name.split('.')[0]}_{DETECTORSEED:06}.root"
target_bootstrap = f'/cr/work/filip/Simulations/Bootstraps/bootstrap_{proc_no}.xml'

# Prepare bootstrap
replacements = {
    '@OUTPUTFILE@' : target_file,
    '@INPUTFILE@' : source_file,
    '@DETECTORSEED@' : f"{DETECTORSEED:06}",
    '@PHYSICSSEED@' : f"{PHYSICSSEED:06}",
    '@PATTERNPATH@' : '*',                      # TODO: will fail for some shit
    '@GROUNDDATA@' : '(1)'                      # TODO: will fail for some shit
}

offline_config_path = "/cr/data01/filip/offline/bd0e9e/install/share/auger-offline/config"
replace_config_dir = lambda x: x.replace("@CONFIGDIR@", offline_config_path)
with open(target_bootstrap, 'w') as target:
    with open('./bootstrap.xml', 'r') as source:
        for line in source.readlines():
            try:
                target.write(replacements[line.strip()] + "\n")
            except KeyError:
                target.write(replace_config_dir(line))

subprocess.call(["./run_simulation.sh", target_bootstrap, target_file])