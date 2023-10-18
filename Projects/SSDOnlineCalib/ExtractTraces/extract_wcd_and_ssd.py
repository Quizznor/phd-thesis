#!/bin/python3

import os, sys
import numpy as np

write_timestamps = False
write_WCD = True
write_SSD = False
do_filtering = True
do_downsampling = True

def apply_downsampling(pmt : np.ndarray) -> np.ndarray :
    random_phase = np.random.randint(3)

    return np.array([pmt[i] for i in range(random_phase, len(pmt), 3)])

def apply_filtering(pmt : np.ndarray) -> np.ndarray :

    # see Framework/SDetector/UUBDownsampleFilter.h in Offline main branch for more information
    kFirCoefficients = [ 5, 0, 12, 22, 0, -61, -96, 0, 256, 551, 681, 551, 256, 0, -96, -61, 0, 22, 12, 0, 5 ]
    buffer_length = int(0.5 * len(kFirCoefficients))
    kFirNormalizationBitShift = 11

    temp = np.zeros(len(pmt) + len(kFirCoefficients))
    trace = np.zeros(len(pmt))

    temp[0 : buffer_length] = pmt[:: -1][-buffer_length - 1 : -1]
    temp[-buffer_length - 1: -1] = pmt[:: -1][0 : buffer_length]
    temp[buffer_length : -buffer_length - 1] = pmt

    # perform downsampling
    for j, coeff in enumerate(kFirCoefficients):
        trace += [temp[k + j] * coeff for k in range(0, len(pmt), 1)]

    # clipping and bitshifting
    trace = [int(adc) >> kFirNormalizationBitShift for adc in trace]

    # Simulate saturation of PMTs at 4095 ADC counts ~ 19 VEM <- same for HG/LG? I doubt it
    return np.array(trace)

station, date, _id = sys.argv[1:]

# only loop through one file
working_dir = f"/cr/tempdata01/filip/UubRandoms/{date}/raw/{station}/"
file = os.listdir(working_dir)[int(_id)]

if do_filtering: station += "Filtered"
if do_downsampling: station += "Downsampled"
timestamps = []

os.system(f"mkdir -p /cr/tempdata01/filip/UubRandoms/{date}/converted/{station}")
wcd_file = f"/cr/tempdata01/filip/UubRandoms/{date}/converted/{station}/{file.replace('.dat','_WCD')}.dat"
ssd_file = f"/cr/tempdata01/filip/UubRandoms/{date}/converted/{station}/{file.replace('.dat','_SSD')}.dat"
timestamp_file = f"/cr/tempdata01/filip/UubRandoms/{date}/converted/timestamps/{station}.dat"

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

                if do_filtering: pmt = apply_filtering(pmt)
                if do_downsampling: pmt = apply_downsampling(pmt)

                with open(wcd_file, "a") as WCD:
                    # b = b''
                    # for _bin in pmt:
                    #     this_number = int(_bin) - int(baseline)
                    #     b += this_number.to_bytes(2, 'little', signed = True)
                    # b += os.linesep.encode('utf-8')
                    WCD.write(" ".join([str(int(_bin - baseline)) for _bin in pmt]) + "\n")

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