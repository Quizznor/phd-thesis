import os
import pandas as pd

os.system('rm -rf /home/filip/Desktop/xy-quality-files/*')

home_dir = '/home/filip/'

status = {}
all, failed = 0, 0
for file in os.listdir(f'{home_dir}/xy-calibration/config/calib_runlists'):
    df = pd.read_csv(f'{home_dir}/xy-calibration/config/calib_runlists/{file}', delimiter=';', comment='#', 
                     names=('runid', 'tel', 'step', 'date', 'src', 'mA', 'DB', 'jobfile', 'comment'))
    
    for _, run in df.iterrows():
        if not run['DB']: continue
        if "OLO" not in run['src']: continue
        if "reversed positions" in run['comment']: continue
        if run['step'] != 6: continue
        if run['runid'] in ["14453",                     # missing Cal A's
                            "11863",                     # missing Cal A's
                            "15967",                     # god knows what happened
                            "16472",                     # LL6 had some HV issues, super weird results
                            "6101",                      # too few positions, (also analysis fails)
                            ]: continue

        os.chdir(f'{home_dir}/xy-calibration')
        exit_code = os.system(f"./run_Check.py {run['runid']} -v 0 -s -d {home_dir}/Desktop/xy-quality-files")
        
        if status.get(run['tel'].strip(), None) is None:
            status[run['tel'].strip()] = [[run['date'][:-3].strip(), run['runid'], os.WEXITSTATUS(exit_code)]]
        else:
            status[run['tel'].strip()].append([run['date'][:-3].strip(), run['runid'], os.WEXITSTATUS(exit_code)])
        failed += os.WEXITSTATUS(exit_code)
        all += 1

print('\n\n')

for k, v in status.items():
    try:
        v.sort(key=lambda v: v[0])
        for i in range(len(v)):
            v[i][1] = (fr"\fail" if v[i][2] else "\ok") + f"{{{v[i][1]}}}"
        print(k.upper(), "& ".join(*v))
    except TypeError: 
        v[1] = (fr"\fail" if v[2] else "\\ok") + f"{{{v[1]}}}"
        print(k.upper(), v[:-1])
                           
print(f'Accept: {all - failed}/{all}: {(all - failed)/all*1e2:.0f}%')