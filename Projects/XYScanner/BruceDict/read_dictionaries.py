import numpy as np

# Pathlib for path objects for dealing with directory changes between operating systems.
from pathlib import Path
from pathlib import PurePath

# Saving and reading data structures in python

import pickle

# Useful functions

# pretty printing dictionaries
import pprint

pretty_print = pprint.PrettyPrinter().pprint


def find_nearest_index(array, value):
    """
    Finds the nearest index in an array to some value with a buffer.
    """
    # 5 min = 300 seconds
    array = np.asarray(array)
    # If there are two posibilities in an array it selects the lowest index.
    try:
        idx = (np.abs(array - value)).argmin()
        min_sep = (np.abs(array - value)).min()

    except ValueError:
        print(f"Value Error \n Value: {value} \n Array: {array}")

    return idx, min_sep


def find_nearest(array, value):

    idx, min_sep = find_nearest_index(array, value)
    return array[idx], min_sep


def file_str_to_path(file_str):
    """converts file str to path object and also makes parent dirs

    Args:
        file_str (string): file string in absolute path

    Returns:
        file_path: file path object
    """

    file_path = Path(file_str)
    abs_file_path = file_path.resolve()
    abs_file_path.parent.mkdir(parents=True, exist_ok=True)

    return file_path


def abs_file_str_from_file_path(file_path):
    return str(PurePath(file_path))


def save_python_dictionary_from_path(python_dict_file_path, python_dict):

    # python_dict_file_path = file_str_to_path(python_dict_file_str)

    dict_file_absolute_str = abs_file_str_from_file_path(python_dict_file_path)

    print(f"Saving dict:\n {dict_file_absolute_str}")
    with open(python_dict_file_path, "wb") as handle:
        pickle.dump(python_dict, handle, protocol=pickle.HIGHEST_PROTOCOL)

    return dict_file_absolute_str


def save_python_dictionary_from_str(python_dict_file_str, python_dict):

    python_dict_file_path = file_str_to_path(python_dict_file_str)

    with open(python_dict_file_path, "wb") as handle:
        pickle.dump(python_dict, handle, protocol=pickle.HIGHEST_PROTOCOL)

    return


def load_python_dictionary_from_str(python_dict_file_str):

    if python_dict_file_str != None:
        python_dict_file_path = file_str_to_path(python_dict_file_str)

        with open(python_dict_file_path, "rb") as handle:
            python_dict = pickle.load(handle)
    else:
        python_dict = None

    return python_dict


PYTHON_DICTIONARY_DIR = "/Users/bdawson/Dropbox/work/HomeMac/FD_cloud_detection/LICE/make_photon_flux_dictionary/"

photon_flux_file_str = (
    f"{PYTHON_DICTIONARY_DIR}photon-flux-dict_2023-10-06_2023-10-07.pickle"
)
gps_second_file_str = (
    f"{PYTHON_DICTIONARY_DIR}gps-second-dict_2023-10-06_2023-10-07.pickle"
)
calibration_constants_file_str = (
    f"{PYTHON_DICTIONARY_DIR}calibration-constants-dict_2023-10-06_2023-10-07.pickle"
)


photon_flux_dict = load_python_dictionary_from_str(photon_flux_file_str)
gps_second_dict = load_python_dictionary_from_str(gps_second_file_str)
calibration_constants_dict = load_python_dictionary_from_str(
    calibration_constants_file_str
)

# specifying eye and tels to look at
eye_name = "Coihueco"
tel_name = "TelID: 1"

# EX 1. Finding photon flux and gps second closest to a desired gps secondd
desired_gps_second = 1380595306

print(f"desired_gps_second: {desired_gps_second}")

nearest_gps_second_index, min_sep = find_nearest_index(
    gps_second_dict[eye_name][tel_name]["gps-second"], desired_gps_second
)

nearest_gps_second = gps_second_dict[eye_name][tel_name]["gps-second"][
    nearest_gps_second_index
]

nearest_photon_flux = photon_flux_dict[eye_name][tel_name]["photon-flux"][
    nearest_gps_second_index
]

nearest_calibration_constants = calibration_constants_dict[eye_name][tel_name][
    "calibration-constants"
][nearest_gps_second_index]

print(f"nearest_gps_second: {nearest_gps_second}")
print(f"abs(nearest_gps_second-desired_gps_second): {min_sep}")

# Photon flux at specific pixel
pixel_id = 200  # ranges between [1,440]

print(f"photon flux at pixel id {pixel_id}: {nearest_photon_flux[pixel_id-1]}")
print(
    f"calibration constant at pixel id {pixel_id}: {nearest_calibration_constants[pixel_id-1]}"
)

# Other dictionaries also call value using dictionary name i.e calibration_constants dict uses 'calibration_constants'

# pretty way of printing the entire dictionary - may provide insight into its structure
# pretty_print(photon_flux_dict)
# pretty_print(gps_second_dict)

# Structure follows something like:
# photon_flux_dict = {
#     "Coihueco": {
#         "TelID: 1" : {
#             [[photon_flux_values_at_gps_second_1], ... , photon_flux_values_at_gps_second_n]
#         },
#         ⋮
#         "TelID: 6" : {
#             [[photon_flux_values_at_gps_second_1], ... , photon_flux_values_at_gps_second_n]
#         }
#     },

#     "HEAT": {
#         "TelID: 1" : {
#             [[photon_flux_values_at_gps_second_1], ... , photon_flux_values_at_gps_second_n]
#         }
#         ⋮
#         "TelID: 3" : {
#             [[photon_flux_values_at_gps_second_1], ... , photon_flux_values_at_gps_second_n]
#         }
# }

# gps_second_dict = {
#     "Coihueco": {
#         "TelID: 1" : {
#             [[gps_second_values_at_gps_second_1], ... , gps_second_values_at_gps_second_n]
#         },
#         ⋮
#         "TelID: 6" : {
#             [[gps_second_values_at_gps_second_1], ... , gps_second_values_at_gps_second_n]
#         }
#     },

#     "HEAT": {
#         "TelID: 1" : {
#             [[gps_second_values_at_gps_second_1], ... , gps_second_values_at_gps_second_n]
#         }
#         ⋮
#         "TelID: 3" : {
#             [[gps_second_values_at_gps_second_1], ... , gps_second_values_at_gps_second_n]
#         }
# }
