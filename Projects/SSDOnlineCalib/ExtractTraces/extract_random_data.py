#!/bin/python3

import os, sys
import numpy as np

# only loop through one file
working_dir = "/cr/tempdata01/filip/iRODS/raw/Nuria/"
file = os.listdir(working_dir)[int(sys.argv[1])]

with open(working_dir + file,"rb") as binary_file:

    with open(f"/cr/data01/filip/SSDCalib/{file.replace('.dat','_WCD')}.dat", "x") as _: pass
    with open(f"/cr/data01/filip/SSDCalib/{file.replace('.dat','_SSD')}.dat", "x") as _: pass 
    
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
        for i in range(len(low_gain)):

            baseline = Baseline[2*i+1 if not Saturated[2*i] else 2*i]
            pmt = high_gain[i] if not Saturated[2*i] else low_gain[i]
            # pmt -= Baseline[2*i+1 if not Saturated[2*i] else 2*i]         # perform Online baseline estimation

            # baselines_wcd[i].append(Baseline[2*i+1 if not Saturated[2*i] else 2*i])
            # first_bins_wcd[i].append(pmt[0])

            with open(f"/cr/data01/filip/SSDCalib/{file.replace('.dat','_WCD')}.dat", "a") as WCD:
                WCD.write(f"{int(baseline)} " + " ".join([str(int(_)) for _ in pmt]) + "\n")

    # save SSD traces
    with open(f"/cr/data01/filip/SSDCalib/{file.replace('.dat','_SSD')}.dat", "a") as SSD:
        ssd_baseline = int(np.floor(np.mean(first_bins_ssd)))
        for trace in all_ssd_traces:
            SSD.write(f"{ssd_baseline} " + " ".join([str(int(_)) for _ in trace]) + "\n")


'''
c = ["steelblue", "orange", "red"]
bins = np.linspace(150, 280, 130)

for i in range(3):
    plt.hist(baselines_wcd[i], histtype="step", color=c[i], bins=bins)
    plt.hist(first_bins_wcd[i], histtype="step", color=c[i], ls="--", bins=bins)
    plt.axvline(np.mean(first_bins_wcd[i]), color=c[i], ls="--", lw=2)
    plt.axvline(np.mean(baselines_wcd[i]), color=c[i], lw=2)

plt.title(file)
plt.hist([], ls="--", color="k", label="first bins in random trace", histtype="step")
plt.hist([], ls="solid", color="k", label="online baseline estimate", histtype="step")
plt.legend()

plt.xlim(220, 280)
plt.savefig("test")
'''