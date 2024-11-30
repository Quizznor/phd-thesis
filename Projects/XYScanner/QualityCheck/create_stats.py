import os
import pandas as pd

home_dir = '/home/filip/'

os.chdir(f'{home_dir}/xy-calibration')
for file in os.listdir('config/calib_runlists'):
    df = pd.read_csv(f'{home_dir}/xy-calibration/config/calib_runlists/{file}', delimiter=';', comment='#', 
                     names=('runid', 'tel', 'step', 'date', 'src', 'mA', 'DB', 'jobfile', 'comment'))
    
    for _, run in df.iterrows():
        if not run['DB']: continue
        if "OLO" not in run['src']: continue
        if "reversed positions" in run['comment']: continue
        if run['runid'] in ["14453",                     # missing Cal A's
                            "16478",                     # LL6 had some HV issues, super weird results
                            
                            ]: continue                  # reject missing Cal A's

        
        os.system(f"./run_Check.py {run['runid']} -s -d {home_dir}/Desktop/xy-quality-files")