import os
import socket

hname = socket.gethostname()

# directory of the data
if hname == "crcds44":
    dp_str = '/cr/users/schaefer-c/xy/data/'
elif hname == "solaire":    # christoph's laptop
    dp_str = '/home/christoph/xy/data/'
elif hname == 'beep-boop':  # paul's laptop
    dp_str = '/home/quizznor/xy-data/'
elif hname == 'crcws02':    # working on IAP cluster
    dp_str = '/cr/data02/AugerPrime/XY/'
else:
    print(f"unkown host: {hname}")
    exit()


#################
if 0: # TESTING #
    #############
    run_list = 'calib_runs_va.list'
    source_radiance = {  # [gamma sr^-1 m^-2]
        "KIT": 300e9,  # test
        "OLO": 300e9,  # test
        "TUB": 100e9,  # test
    }
    dp_str += 'va/'
    
##################################
elif 0:  # NOVEMBER 2023 campaign #
    ###############################
    run_list = 'calib_runs_2023-11.list'
    source_radiance = {  # [gamma sr^-1 m^-2]
        # uncertainty of the source radiance is 2.8%
        # source radiance measurements all performed at KIT
        "KIT": 339e9,  # KIT_sphere_distAbsCalib_022_MUG6           28.09.2023
        "OLO": 373e9,  # OLO_sphere_distAbsCalib_033_MUG6           26.09.2023
    }
    dp_str += '2023nov/'

#################################
elif 0:  # OCTOBER 2023 campaign #
    ##############################
    run_list = 'calib_runs_2023-10.list'
    source_radiance = {  # [gamma sr^-1 m^-2]
        # uncertainty of the source radiance is 2.8%
        # source radiance measurements all performed at KIT
        "KIT": 339e9,  # KIT_sphere_distAbsCalib_022_MUG6           28.09.2023
        "OLO": 373e9,  # OLO_sphere_distAbsCalib_033_MUG6           26.09.2023
    }
    dp_str += '2023oct/'

##################################
elif 0:  # OCTOBER 2022 campaign #
    ##############################
    run_list = 'calib_runs_2022-10.list'
    source_radiance = {  # [gamma sr^-1 m^-2]
        # uncertainty of the source radiance is 2.8%
        # source radiance measurements all performed at KIT
        # "KIT" : 336e9,   # KIT_sphere_distAbsCalib_008_MUG6            24.01.2023
        "KIT": 335e9,  # KIT_sphere_distAbsCalib_019_MUG6_keithIPE   27.03.2023
        # "OLO" : 370e9,   # OLO_sphere_distAbsCalib_019_MUG6            10.03.2023
        "OLO": 368e9,  # OLO_sphere_distAbsCalib_026_MUG6_keithIPE   29.03.2023
        "TUB": 121e9,  # tube_source_distAbsCalib_004_MUG6           16.03.2023
    }
    dp_str += '2022oct/'
###################################
elif 0:  # NOVEMBER 2019 campaign #
    ###############################
    run_list = 'calib_runs_2019-11.list'
    source_radiance = {  # [gamma sr^-1 m^-2]
        "KIT": 335e9,  # same source radiance for for OCT 2022
        "OLO": 368e9,  # results from Wuppertal agree with KIT setup...
    }
    dp_str += '2019nov/'
else:
    print("[ERROR] No campaign selected in 'config.py'!!")
    exit()

# path to store and read data
data_path = os.path.normpath(dp_str)
if not os.path.isdir(data_path):
    print(f"[INFO] directory '{data_path}' does not exist")
    os.makedirs(data_path)
    print(f"[INFO] created directory '{data_path}'")


source_radius = {
    "KIT": 5.08 / 2,  # cm
    "OLO": 5.08 / 2,  # cm
    "TUB": 9.73 / 2,  # cm
}

pf = 1  # run pulse finder     (default = 1)
details = 1  # dump traces and so.. (default = 1)
