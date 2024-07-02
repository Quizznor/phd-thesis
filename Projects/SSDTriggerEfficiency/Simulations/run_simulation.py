#!/usr/bin/python3

import pathlib
import os, sys
import subprocess

# TODO: grab source file

source_file = "/lsdf/auger/corsika/prague/QGSJET-II.04/proton/18_18.5/DAT010002"
source_name, proc_no = source_file.split('/')[-1], sys.argv[1]

target_file = f"/cr/tempdata01/filip/SSDTriggerEfficiency/traces/run01/{source_name.split('.')[0]}_{proc_no}.root"
target_bootstrap = f'/cr/work/filip/Simulations/Bootstraps/bootstrap_{proc_no}.xml'

# Prepare bootstrap
replacements = {
    '@OUTPUTFILE@' : target_file,
    '@INPUTFILE@' : source_file,
    '@DETECTORSEED@' : f"{proc_no:06}",
    '@PHYSICSSEED@' : f"{proc_no:06}",
    '@PATTERNPATH@' : '*',                      # TODO: will fail for some shit
    '@GROUNDDATA@' : '(1)'                      # TODO: will fail for some shit
}


with open(target_bootstrap, 'w') as target:
    with open('./bootstrap.xml.in', 'r') as source:
        for line in source.readlines():
            try:
                target.write(replacements[line.strip()] + "\n")
            except KeyError:
                target.write(line)

subprocess.call(["./run_simulation.sh", target_bootstrap])

# # LIBRARIES:
# # 15.5 - 18.5 log E = /lsdf/auger/corsika/prague/QGSJET-II.04/proton/ -    NO EXTENSION
# # 18.5 - 20.2 log E = /lsdf/auger/corsika/napoli/QGSJET-II.04/proton/ - .part EXTENSION

# E_DICT = {
#           "16_16.5" : ["prague","*","(1)"],
#           "16.5_17" : ["prague","*","(1)"],
#           "17_17.5" : ["prague","*","(1)"],
#           "17.5_18" : ["prague","*","(1)"],
#           "18_18.5" : ["prague","*","(1)"],
#           "18.5_19" : ["napoli","*.part","(1).part"],
#           "19_19.5" : ["napoli","*.part","(1).part"],
#           }

# E_RANGE = "17_17.5"
# ALREADY_PRESENT = 0
# NUM_RETHROWS = 2

# SRC_DIR=f"/lsdf/auger/corsika/{E_DICT[E_RANGE][0]}/QGSJET-II.04/proton/{E_RANGE}/"
# # SRC_DIR=f"/cr/users/filip/Simulation/TestOutput/"
# DESTINATION_DIR=f"/cr/tempdata01/filip/QGSJET-II/LTP/{E_RANGE}/"
# file_list = [file for file in os.listdir(SRC_DIR) if not '.' in file or file.endswith(".part")]
# FILE_NAME = file_list[int(sys.argv[1])]
# EVENT_NAME = FILE_NAME.replace(".part", "")

# for j in range(ALREADY_PRESENT, ALREADY_PRESENT + NUM_RETHROWS):
#     NAME = f"{EVENT_NAME}"
#     SEED = str(j).zfill(6)

#     try:
#         # if os.path.isfile(f"{DESTINATION_DIR}/{NAME}.csv"):
#         #     raise IndexError

#         bash_arguments = file_list[int(sys.argv[1])], SRC_DIR, DESTINATION_DIR, NAME, E_RANGE, SEED, E_DICT[E_RANGE][1], E_DICT[E_RANGE][2]
#         subprocess.call([f"/cr/users/filip/Simulation/calculateLTP/run_simulation.sh", *bash_arguments])

#     except IndexError:
#         pass