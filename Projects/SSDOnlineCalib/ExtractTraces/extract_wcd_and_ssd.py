#!/bin/python3

import os, sys
import numpy as np

write_timestamps = False
write_WCD = True
write_SSD = False
apply_filtering = True
apply_downsampling = False

# only loop through one file
working_dir = "/cr/tempdata01/filip/iRODS/UubRandoms/raw/Nov2022/" + sys.argv[1] + "/"
file = os.listdir(working_dir)[int(sys.argv[2])]
timestamps = []

os.system(f"mkdir /cr/tempdata01/filip/iRODS/UubRandoms/converted/{sys.argv[1]}")
wcd_file = f"/cr/tempdata01/filip/iRODS/UubRandoms/converted/{sys.argv[1]}/{file.replace('.dat','_WCD')}.dat"
ssd_file = f"/cr/tempdata01/filip/iRODS/UubRandoms/converted/{sys.argv[1]}/{file.replace('.dat','_SSD')}.dat"
timestamp_file = f"/cr/tempdata01/filip/iRODS/UubRandoms/converted/timestamps/{sys.argv[1]}.dat"

# implement check to scan for files that are alrady present
with open(working_dir + file,"rb") as binary_file:

    if write_WCD:
        with open(wcd_file, "w+") as _: pass
    
    block_size, endianness = 4, "little"
    read_to_int = lambda : int.from_bytes(binary_file.read(block_size), endianness)

    baselines_wcd = [[] for _ in range(3)]
    first_bins_wcd = [[] for _ in range(3)]
    first_bins_ssd = []
    all_ssd_traces = []

    while True:
        # time_stamp = binary_file.read(block_size)           # second timestamp, don't really need this
        # time_delta = binary_file.read(block_size)           # time delta between data taking(?), not sure
        time_stamp = read_to_int()
        time_delta = read_to_int()

        # stop when reaching the end of the file
        if not time_delta != 0:
            break

        timestamps.append(time_stamp)

        # read trace metadata
        n_metadata  = 10                                    # 2 (LG, HG) * number of PMTs in station, 3 WCD, etc.
        Saturated   = np.zeros(n_metadata)                  # whether the low/high gain is saturated       
        Baseline    = np.zeros_like(Saturated)              # mean ADC count of this trace (roughly)
        Peak        = np.zeros_like(Saturated)              # VEM_peak value to convert from ADC to VEM
        Area        = np.zeros_like(Saturated)              # Area under signal or something like that

        for i in range(len(Saturated)):
            index_buffer = read_to_int()                    # this is not important for analysis
            Saturated[i] = read_to_int()
            Baseline[i] = read_to_int() >> 4                # baseline is given as baseline * 16 according to Daves' mail
            Peak[i] = read_to_int()                         # different from the 215 point something in simulations !!
            Area[i] = read_to_int()

        # read trace values
        n_bins          = 2048
        low_gain    = np.zeros((3,n_bins))
        high_gain   = np.zeros((3,n_bins))
        ssd_buffer      = np.zeros(n_bins)

        for i in range(n_bins):
            index = read_to_int()                           # not useful in this analysis

            buffer = [ read_to_int() for i in range(3) ]    # 3 pmts in the WCD

            small_pmt = read_to_int()                       # disregard small pmt (no HG)
            ssd_pmt = read_to_int()                         # disregard surface station 
            rd_temp =  read_to_int()                        # disregard radio info

            for j in range(len(buffer)):
                low_gain[j][i] = buffer[j] & 0xfff
                high_gain[j][i] = (buffer[j] >> 16) & 0xfff

            ssd_buffer[i] = (ssd_pmt >> 16) & 0xfff 
            if i == 0: first_bins_ssd.append((ssd_pmt >> 16) & 0xfff)

        all_ssd_traces.append(ssd_buffer)

        # save WCD traces
        if write_WCD:
            for i in range(len(low_gain)):

                baseline = Baseline[2*i+1 if not Saturated[2*i] else 2*i]
                pmt = high_gain[i] if not Saturated[2*i] else low_gain[i]

                with open(wcd_file, "a") as WCD:
                    WCD.write(f"{int(baseline)} " + " ".join([str(int(_)) for _ in pmt]) + "\n")

    # save SSD traces
    if write_SSD:
        with open(ssd_file, "w+") as SSD:
            ssd_baseline = int(np.floor(np.mean(first_bins_ssd)))
            for trace in all_ssd_traces:
                SSD.write(f"{ssd_baseline} " + " ".join([str(int(_)) for _ in trace]) + "\n")

    # save timestamps
    if write_timestamps:
        with open(timestamp_file, "a") as f:
            f.write(f"{file.split('/')[-1]} {min(timestamps)} {max(timestamps)}\n")