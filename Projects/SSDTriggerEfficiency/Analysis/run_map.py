#!/usr/bin/python3

from itertools import product
from Event import Event
import pandas as pd
import numpy as np
import sys, os

src_dir = f'/cr/work/filip/SSDTriggerEfficiency/{sys.argv[1]}/'
proc_no = int(sys.argv[2])

file = os.listdir(src_dir)[proc_no]
event = Event(src_dir + file)
multiplicities = range(5, 45)
threshold = np.linspace(0.05, 3.00, 295)
all_params = list(product(multiplicities, threshold))

filter = lambda col: int(col) < 750

def read():

    base = '/cr/data01/filip/Data/SSDtotRateMap/'
    data = np.loadtxt(f'{base}/Nadia_SSD.txt', usecols=[0, 1, 2, 4])
    container = {}

    for row in data:
        x_, y_, z_, t = row
        container[(x_, y_)] = z_ / t

    assert len(DAQ_seconds := np.unique(data[:, 3])) == 1, f"DAQ time(s) do not match: {DAQ_seconds = }"

    return container

trigger_list = []
ssd_background_rates = read()
# for params in tools.ProgressBar(all_params):
for params in all_params:
    if ssd_background_rates[params] > 50: continue

    class cfg_class():

        threshold : dict = {
            'wcd_tot' : 0.2,
            'ssd_tot' : params[1],
            'wcd_th'  : 3.2,
        }

        occupancy : dict = {
            'wcd' : 12,
            'ssd' : params[0],
        }

        multiplicity : dict = {
            'wcd' : 1,
            'ssd' : 0,
        }

    param_list = [params]
    cfg = cfg_class()
    for station in event.stations:
        isToT_SSD = int(station.isToT_SSD(cfg) != -1)
        isWCDT2 = int(station.isWCDT2(cfg) != -1)
        param_list.append(isToT_SSD - isWCDT2)

    trigger_list.append(param_list)

df = pd.DataFrame(trigger_list, columns=['params'] + event.get_spds())
df.set_index('params', inplace=True)

cols_close = [col for col in df.columns if filter(col)]
df_close = df[cols_close]
df_close['sum_close'] = df_close.sum(axis=1, numeric_only=True)
df_close['n_close'] = len(cols_close)
cols_far = [col for col in df.columns if not filter(col)]
df_far = df[cols_far]
df_far['sum_far'] = df_far.sum(axis=1, numeric_only = True)
df_far['n_far'] = len(cols_far)

df_new = pd.concat([df_close[["sum_close", "n_close"]], df_far[["sum_far", "n_far"]]], axis=1)
df_new.to_csv(f'{src_dir}/../{sys.argv[1]}_evaluated/{file}')