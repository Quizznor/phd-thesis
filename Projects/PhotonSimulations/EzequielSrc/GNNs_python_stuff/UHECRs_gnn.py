# Standard library imports
import json
import os
import random
import sys
from pprint import pprint
from datetime import datetime
import xml.etree.ElementTree as ET
from abc import ABCMeta, abstractmethod
import concurrent
from typing import List, Optional, Dict, Tuple
from collections import defaultdict
import warnings

# Third party imports
import numpy as np
import pandas as pd
import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import random_split, Dataset
import torch_geometric
from torch_geometric.data import Dataset, Data, Batch, HeteroData
from torch_geometric.nn.norm import BatchNorm, HeteroBatchNorm
from torch_geometric.loader import DataLoader
from torch_geometric.transforms import BaseTransform
from torch_geometric.utils import subgraph, dropout_node
from torch_geometric.nn.conv import GATv2Conv, HANConv
from torch_geometric.nn.pool import global_mean_pool, global_max_pool
from scipy.signal import convolve
from mpl_toolkits.axes_grid1.inset_locator import Bbox, inset_axes
from sklearn.model_selection import StratifiedShuffleSplit
from tqdm import tqdm

##############
## Datasets ##
##############


class SD433UMDatasetPyG(Dataset, metaclass=ABCMeta):
    """
    Custom PyTorch Geometric dataset for SD433-UMD simulations. 
    This class serves as a base class for processing and storing data streamed from ADSTs to JSON files. 
    It is not meant to be instantiated directly, but instead serves as a foundation for subclasses that allow 
    for diverse event representation.

    Args:
        file_paths (List[str]): 
            List of file paths pointing to .json files containing the raw dataset.
        root (str): 
            Root directory for storing processed data.
        normalization_dict (dict, optional): 
            A dictionary containing normalization parameters for various features. Default is an empty dictionary.
            Structure should be:
            {
                'feature_name': {
                    'method': 'normalization_method',  # e.g., 'min_max_scaling' or 'standardization'
                    'mean': <mean_value>,               # required for standardization
                    'std': <std_value>,                 # required for standardization
                    'min': <min_value>,                 # required for min-max scaling
                    'max': <max_value>                  # required for min-max scaling
                }
            }
    """

    def __init__(self, file_paths: List[str], root: str, normalization_dict: dict = {},
                isData: bool = False, offgrid: bool = True):
        self.file_paths = file_paths
        self.id_list = [
            os.path.basename(raw_path.replace(".json", ""))
            for raw_path in self.file_paths
        ]
        self.error_set = set()
        self.n_time_bins = 120
        self.normalization_dict = normalization_dict or {}
        
        # to parse the station list from XML file
        tree = ET.parse('/sps/pauger/users/erodriguez/Software/offline_install/share/auger-offline/config/SStationList.xml')  # Replace with the correct path to your XML file
        self.station_list_root = tree.getroot()
        self.isData = False # we are using simulations
        self.offgrid = offgrid

        self.sd433_data_ids = np.array([# SD-433
                            #central station
                            1764,
                            # 1st crown
                            97, 13, 47, 99, 11, 98, 30, 12,
                            # 2nd crown
                            28, 1769, 54, 1765, 50, 734, 42, 1773, 27, 1622, 29, 688])
        self.sd750_data_ids = np.array([# SD-750
                            # 1st crown around SD-433
                            1760, 1767, 669, 1763, 1774, 1816, 1813, 1811, 93, 1574, 1570, 1761])
                            # 2nd crown around SD-433
                            #698, 1776, 1770, 1766, 1771, 819, 1810, 643, 1825, 651, 1826, 1812, 1625, 1627, 701, 1762, 702, 1777])
        
        self.station_features = [
                    "x",
                    "y",
                    "z",
                    "deltaTimeHottest",
                    "pmt_number",
                    "WCD_signal",
                    "effective_area",
                    "rho_mu",
                ]
        self.n_features = len(self.station_features)
        super().__init__(root)

    @property
    def raw_file_names(self) -> List[str]:
        """
        Return the list of raw file names.

        Returns:
            List[str]: A list of file paths to the raw .json files.
        """
        return self.file_paths

    @property
    def processed_file_names(self) -> List[str]:
        """
        Return the list of processed file names.

        Returns:
            List[str]: A list of file paths to the processed .pt files.
        """
        return [os.path.join(self.processed_dir, f"{id_}.pt") for id_ in self.id_list]

    def len(self) -> int:
        """
        Return the number of processed files.

        Returns:
            int: The count of processed .pt files.
        """
        return len(self.processed_file_names)

    def process(self):
        """
        Process the raw data files and save the processed data in PyTorch format.

        The method reads raw data from the specified .json files, processes it into the appropriate 
        format, and saves the results as .pt files. Processing is done concurrently to improve efficiency.
        Any errors encountered during processing will be recorded.
        """

        # Define a function to process each file
        def process_file(raw_path, self):
            try:
                # Read data from raw_path
                with open(raw_path) as _json_file:
                    json_file = json.load(_json_file)

                # Order stations by distance to the hottest station, considering only non-rejected stations
                station_list = [key for key in list(json_file.keys()) if (("sd_" in key) and (json_file[key]["status"] != "Reje"))] # these are the candidates
                rejected_list = [key for key in list(json_file.keys()) if (("sd_" in key) and (json_file[key]["status"] == "Reje"))]
                # getting silent ids from missing stations in the candidate and rejected list
                if not self.isData:
                    # let's use the latest configuration for simulations
                    # I should implement a the possibility for data in the future
                    event_datetime = datetime.strptime("2018-10-10T00:00:00Z", "%Y-%m-%dT%H:%M:%SZ") 
                
                valid_ids = self._exclude_ids_based_on_event(True, event_datetime, self.sd433_data_ids, self.sd750_data_ids, self.offgrid) # should fix this in the future
                candidate_ids = [int(sd_id.split(sep='_')[1])-4000 if int(sd_id.split(sep='_')[1])>4000 else int(sd_id.split(sep='_')[1]) for sd_id in station_list]
                rejected_ids = [int(sd_id.split(sep='_')[1])-4000 if int(sd_id.split(sep='_')[1])>4000 else int(sd_id.split(sep='_')[1]) for sd_id in rejected_list]
                silent_ids = (set(valid_ids).difference(set(candidate_ids))).difference(rejected_ids)
                
                # Distance to hottest station
                id_hottest = str(json_file["hottest_id"])
                hottest_pos = np.array(self._get_pos_from_xml(int(id_hottest)-4000), dtype=float)
                hottest_time_S = json_file["sd_" + id_hottest]["seconds"]
                hottest_time_NS = json_file["sd_" + id_hottest]["nanoSeconds"]
                hottest_distance_list = [
                    np.sqrt(np.sum((hottest_pos - np.array(self._get_pos_from_xml(int(station_id.split(sep='_')[-1])-4000), dtype=float))** 2))
                    for station_id in station_list
                ]
                
                # Order station list by distance to the hottest
                pairs = zip(hottest_distance_list, station_list)
                sorted_pairs = sorted(pairs, key=lambda x: x[0])  # Sort the pairs based on distance
                hottest_distance_list, station_list = zip(*sorted_pairs)  # Unpack the sorted pairs

                threshold_distance_close, threshold_distance_far = (50, 760)  # Threshold value in meters
                station_list = [
                    station_id for station_id, distance in zip(station_list, hottest_distance_list)
                ]

                distance2hottest_list = [
                    distance
                    for station_id, distance in zip(station_list, hottest_distance_list)
                ]

                n_stations = len(station_list)
                x = np.empty((n_stations, self.n_features), float)
                pos = np.empty((n_stations, 2), float)
    
                ###########################
                ## Features and Position ##
                ###########################

                for ids, station in enumerate(station_list):
                    rel_pos = hottest_pos - np.array(self._get_pos_from_xml(int(station.split(sep='_')[-1])-4000), dtype=float)
                    json_file[station]["x"] = rel_pos[0]
                    json_file[station]["y"] = rel_pos[1]
                    json_file[station]["z"] = rel_pos[2]
                    json_file[station]["deltaTimeHottest"] = ((hottest_time_S - json_file[station]["seconds"]) * 1e9 +
                                                              (hottest_time_NS - json_file[station]["nanoSeconds"]))
                    json_file[station]["pmt_number"] = 3
                    json_file[station]["WCD_signal"] = json_file[station]["totalSignalCalc"]
                    # MD part
                    md_module_list = [key for key in list(json_file[station].keys()) if ("md_" in key)]
                    json_file[station]["effective_area"] = np.sum([json_file[station][md_id]["effective_area"] for md_id in md_module_list])
                    json_file[station]["rho_mu"] = ((np.sum([json_file[station][md_id]["n_estimated_muons"] for md_id in md_module_list])) /
                                                    json_file[station]["effective_area"])

                    # Append station-level features
                    x_ = [json_file[station][feature] for feature in self.station_features]
                    # Traces
                    x[ids, :] = np.array(x_, dtype=float)
                    # Positions
                    pos[ids, :] = np.array([self._get_pos_from_xml(int(station.split(sep='_')[-1])-4000)[0],
                                            self._get_pos_from_xml(int(station.split(sep='_')[-1])-4000)[1]], dtype=float)
                
                # we only save the relative positions to hottest
                # the other features and "x_traces_silent" are set in the get method
                x_silent = np.empty((len(silent_ids), 3), float)
                pos_silent = np.empty((len(silent_ids), 2), float)
                # getting position fror silent stations
                for ids, station in enumerate(silent_ids):
                    abs_pos = np.array(self._get_pos_from_xml(int(station)))
                    x_silent[ids, :] = np.array(hottest_pos-abs_pos, dtype=float)
                    pos_silent[ids, :] = abs_pos[:2]
                
                x = torch.tensor(x, dtype=torch.float)
                pos = torch.tensor(pos, dtype=torch.float)
                x_silent = torch.tensor(x_silent, dtype=torch.float)
                pos_silent = torch.tensor(pos_silent, dtype=torch.float)
                
                # Initialize arrays for traces, total signals
                x_traces = np.zeros((len(station_list), 3, self.n_time_bins), dtype=np.float32)
                total_signal = np.zeros((len(station_list), 3), dtype=np.float32)
                effective_area = np.zeros((len(station_list), 3), dtype=np.float32)
                md_Nmu = np.zeros((len(station_list), 3), dtype=np.float32)

                # Populate arrays
                for ids, station in enumerate(station_list):
                    for pmt_module_id in range(3):
                        x_traces[ids, pmt_module_id] = ((json_file[station][f"pmt_{pmt_module_id + 1}"]["peak"] /
                                                        json_file[station][f"pmt_{pmt_module_id + 1}"]["charge"]) * 
                        (np.array(json_file[station][f"pmt_{pmt_module_id + 1}"]["trace"])))
                        total_signal[ids, pmt_module_id] = json_file[station][f"pmt_{pmt_module_id + 1}"]["totalSignal"]
                        effective_area[ids, pmt_module_id] = json_file[station][f"md_{pmt_module_id + 1}"]["effective_area"]
                        md_Nmu[ids, pmt_module_id] = json_file[station][f"md_{pmt_module_id + 1 }"]["n_estimated_muons"]

                data = Data(x=x, pos=pos)

                #############################
                ## End of Graph Definition ##
                #############################

                data.id = os.path.basename(raw_path.replace(".json", ""))
                data.y = 1 if (json_file["primary"] == "Photon") else 0
                data.x_traces = torch.tensor(np.clip(x_traces, -0.99, a_max=None), dtype=torch.float)
                data.total_signal = torch.tensor(total_signal)
                data.effective_area = torch.tensor(effective_area)
                data.md_Nmu = torch.tensor(md_Nmu)
                data.station_list = candidate_ids
                data.rejected_list = rejected_list
                data.core = (hottest_pos[0] - json_file["coreX_MC"], hottest_pos[1] - json_file["coreY_MC"])
                data.distance2hottest_list = distance2hottest_list
                data.seed = json_file["seed_stations"]
                data.Xmax = json_file["Xmax"]
                data.Nmu = json_file["muon_number"]
                data.energy_MC = json_file["energy_MC"]
                data.electromagnetic_energy = json_file["electromagnetic_energy"]
                # silent
                data.pos_silent = pos_silent
                data.x_silent = x_silent
                data.silent_list = list(silent_ids)

                torch.save(data, os.path.join(self.processed_dir, f"{data.id}.pt"))

            except Exception as e:
                print(f"Error processing file {raw_path}: {e}")
                self.error_set.add(raw_path)

        # Initialize ThreadPoolExecutor
        with concurrent.futures.ThreadPoolExecutor() as executor:
            # Submit tasks for each raw file
            futures = [
                executor.submit(process_file, raw_path, self)
                for raw_path in tqdm(self.raw_file_names)
            ]

            # Wait for all tasks to complete
            for future in concurrent.futures.as_completed(futures):
                if future.exception() is not None:
                    print(f"Error occurred: {future.exception()}")

        self.file_paths = [path for path in self.raw_file_names if path not in self.error_set]
        self.id_list = [
            os.path.basename(raw_path.replace(".json", ""))
            for raw_path in self.file_paths
        ]

    @abstractmethod
    def get(self, idx) -> Data:
        """
        Abstract method to retrieve a data sample.

        Args:
            idx (int): Index of the sample to retrieve.

        Returns:
            Data: The data sample corresponding to the specified index.
        """
        pass

    def _normalize_feature(self, value: float, feature: str) -> float:
        """
        Normalize a given feature value based on the specified normalization method and parameters.

        Args:
            value (float): 
                The original value of the feature to be normalized.
            feature (str): 
                The name of the feature to normalize.

        Returns:
            float: 
                The normalized value of the feature. If the feature is not in the normalization dictionary, 
                the original value is returned.
        """

        if feature in self.normalization_dict.keys():
            # Get nested dictionary for the specified feature
            the_dict = self.normalization_dict[feature]

            if the_dict["method"] == "min_max_scaling":
                min_value = the_dict["min"]
                max_value = the_dict["max"]
                norm_val = (value - min_value) / (max_value - min_value)

            elif the_dict["method"] == "standardization":
                mean_value = the_dict["mean"]
                std_value = the_dict["std"]
                norm_val = (value - mean_value) / std_value

            else:
                raise ValueError(f"Unsupported normalization method")

            return norm_val

        else:
            return value
        
    def _get_pos(self, json_file, station_id) -> Tuple[float]:
        """
        Return the position of the SD station in the detector reference frame.

        Args:
            json_file (dict): 
                The JSON data structure containing the station information.
            station_id (str): 
                The identifier of the station whose position is to be retrieved.

        Returns:
            Tuple[float]: 
                A tuple containing the x, y, and z coordinates of the station.
        """
        return (
            json_file[station_id]["x"],
            json_file[station_id]["y"],
            json_file[station_id]["z"],
        )

    def _get_pos_from_xml(self, station_id):
        """
        Retrieve the position (northing, easting) of a station by its ID.

        :param station_id: The ID of the station to search for.
        :return: A tuple (easting, northing) if the station is found, otherwise None.
        """
        
        for station in self.station_list_root.findall('station'):
            if station.get('id') == str(station_id):
                northing = float(station.find('northing').text.strip())
                easting = float(station.find('easting').text.strip())
                altitude = float(station.find('altitude').text.strip())
                return easting, northing, altitude 
        return None

    def _exclude_ids_based_on_event(self, isData, event_datetime, sd433_data_ids, sd750_data_ids, offgrid=False):
        # Decomission dates
        chichinojr_1st_decomission = "2017-06-09T21:25:05Z"
        guilijr_1st_decomission = "2017-10-10T00:00:00Z"
        chichinojr_decomission_datetime = datetime.strptime(chichinojr_1st_decomission, "%Y-%m-%dT%H:%M:%SZ")
        guilijr_decomission_datetime = datetime.strptime(guilijr_1st_decomission, "%Y-%m-%dT%H:%M:%SZ")

        exclude_ids = []

        # Determine which IDs to exclude based on the event datetime
        if event_datetime >= chichinojr_decomission_datetime:
            exclude_ids.append(98)
        else:
            exclude_ids.append(30)

        if event_datetime >= guilijr_decomission_datetime:
            exclude_ids.append(13)
        else:
            exclude_ids.append(47)

        # Select which list(s) to check
        if offgrid:
            data_ids = np.concatenate((sd433_data_ids, sd750_data_ids))
        else:
            data_ids = sd433_data_ids

        # Exclude IDs from the selected list(s)
        valid_data_ids = np.array([id for id in data_ids if id not in exclude_ids])

        if isData:
            return valid_data_ids
        else:
            return valid_data_ids + 4000

        
class SD433UMDatasetHomogeneous(SD433UMDatasetPyG):
    """
    Child class of SD433UMDatasetPyG that implements the get method with flexible augmentation options.

    Args:
        file_paths (List[str]): 
            List of file paths pointing to .json files containing data for the dataset.
        root (str): 
            Root directory for storing processed data. This is where the .pt files will be saved or loaded from.
        n_time_bins (int, optional): 
            Number of time bins to use for the traces. Default is 120.
        augmentation_options (dict, optional): 
            A dictionary containing options for data augmentation:
            - 'mask_PMTs' (bool): Whether to apply PMT masking.
            - 'AoP_and_saturation' (bool): Whether to apply AoP transformation and a saturation threshold.
            - 'log_normalize_traces' (bool): Whether to apply logarithmic normalization to the traces.
            - 'mask_MD_mods' (bool): Whether to apply MD module masking.
    """
    
    def __init__(self, file_paths: List[str], root: str, n_time_bins: int = 120,
                 augmentation_options: dict = None, normalization_dict: dict = None, dist_neighbour_cut: float = 790.0, include_silent: bool = True):
        super().__init__(file_paths, root, normalization_dict)
        self.augmentation_options = augmentation_options or {}
        self.n_time_bins = n_time_bins
        self.dist_neighbour_cut = dist_neighbour_cut
        self.include_silent = include_silent
    
    def get(self, idx) -> Data:
        """
        Retrieve the data at the specified index.

        Args:
            idx (int): Index of the data.

        Returns:
            Data: The processed data.
        """

        data = torch.load(os.path.join(self.processed_dir, f"{self.id_list[idx]}.pt"))
        
        ##################
        ## Augmentation ##
        ##################
        
        # SD part - Check if 'mask_PMTs' augmentation is enabled
        if self.augmentation_options.get('mask_PMTs', False):
            # loop through stations
            for ids, station in enumerate(data.station_list):

                PMT_numbers = []
                non_masked_PMT_traces = []
                signals_avgs = []

                for idx in range(len(data.station_list)):
                    n_PMTs = torch.randint(1, 4, (1,))  # uniform sampling to be aggresive!
                    selected_indexes = torch.randperm(3)[:n_PMTs]
                    # Mask the tensor and take the mean
                    non_masked_traces = data.x_traces[idx, selected_indexes, :self.n_time_bins]
                    non_masked_signals = data.total_signal[idx, selected_indexes]
                    mean_trace = torch.mean(non_masked_traces, dim=0)
                    mean_signal = torch.mean(non_masked_signals)
                    # Append the mean to the list
                    PMT_numbers.append(n_PMTs.item())
                    non_masked_PMT_traces.append(mean_trace)
                    signals_avgs.append(mean_signal.item())

            data.x_traces = torch.stack(non_masked_PMT_traces)
            data.x[:, 4] = torch.from_numpy(np.array(PMT_numbers))  # 4 is the column index in the feature matrix
            data.x[:, 5] = torch.from_numpy(np.array(signals_avgs))  # 5 is the column index in the feature matrix
        else:
            # Default behavior when mask_PMTs is not enabled
            data.x_traces = torch.mean(data.x_traces, dim=1)[:, :self.n_time_bins]
        
        # AoP Transformation and Dynamic-Range limitation (fixed saturation threshold)
        if self.augmentation_options.get('AoP_and_saturation', False):
            meanAoP_from_data, stdAoP_from_data = 2.92, 0.2
            AoP_from_sims = 3.2
            AoP_from_sims, augmented_AoP = AoP_from_sims, np.random.normal(loc=meanAoP_from_data, scale=stdAoP_from_data)
            data.x_traces = self._apply_convolution_to_traces(data.x_traces, AoP_from_sims, augmented_AoP)
            data.x_traces = torch.clamp(data.x_traces, max=120.0, min=-0.99) # saturation threshold in VEM charge decided based on data
        
        # trace log normalization (always applied unless specified otherwise in options)
        if self.augmentation_options.get('log_normalize_traces', False):
            data.x_traces = torch.log10(data.x_traces + 1) / np.log10(101)

        if self.augmentation_options.get('log_normalize_signals', True):
            data.x[:, 5] = torch.from_numpy(np.log10(data.x[:, 5].numpy()+1)/ np.log10(101))
        
        # MD part - Check if 'mask_MD_mods' augmentation is enabled
        if self.augmentation_options.get('mask_MD_mods', False):
            # loop through stations
            for ids, station in enumerate(data.station_list):

                effective_area_sum = []
                rho_mu = []

                for idx in range(len(data.station_list)):
                    n_modules = torch.randint(1, 4, (1,)) # uniform sampling to be aggresive!
                    selected_indexes = torch.randperm(3)[:n_modules]
                    # Mask the tensor and sum
                    non_masked_effective_area = data.effective_area[idx, selected_indexes]
                    non_masked_Nmu = data.md_Nmu[idx, selected_indexes]
                    # Mask the tensor and sum
                    non_masked_effective_area_sum = torch.sum(non_masked_effective_area)
                    non_masked_Nmu = torch.sum(non_masked_Nmu)
                    # Append the mean to the list
                    effective_area_sum.append(non_masked_effective_area_sum)
                    rho_mu.append(non_masked_Nmu / non_masked_effective_area_sum)

            data.x[:, 6] = torch.from_numpy(np.array(effective_area_sum))  # 6 is the column index in the feature matrix
            data.x[:, 7] = torch.from_numpy(np.array(rho_mu))  # 7 is the column index in the feature matrix
        
        if self.include_silent: 
            ## Include features for silent stations
            x_silent = torch.cat((data.x_silent, torch.full((data.x_silent.size(0), 1), 0.0)), dim=1) # delta time w.r.t.hottest
            x_silent = torch.cat((x_silent, torch.full((data.x_silent.size(0), 1), 0.0)), dim=1) # number of working PMTs
            x_silent = torch.cat((x_silent, torch.full((data.x_silent.size(0), 1), 0.0)), dim=1) # signal or log10(signal+ 1)
            x_silent = torch.cat((x_silent, torch.full((data.x_silent.size(0), 1), 0.0)), dim=1) # effective area
            x_silent = torch.cat((x_silent, torch.full((data.x_silent.size(0), 1), -1.0)), dim=1) # rho mu
            x_silent_traces = torch.zeros((data.x_silent.size(0), self.n_time_bins), dtype=torch.float32) # the traces
        
            new_x = torch.cat((data.x, x_silent))
            new_data = Data(x=new_x, dim=0, pos=torch.cat((data.pos, data.pos_silent), dim=0))#, edges_index)
            new_data.x_traces = torch.cat((data.x_traces, x_silent_traces), dim=0)
            new_data.station_list = data.station_list + data.silent_list
            new_data.id = data.id
            new_data.y = data.y
            new_data.core = data.core
            new_data.seed = [(station_id - 4000) if (station_id>4000) else station_id for station_id in data.seed]
            new_data.Xmax = data.Xmax
            new_data.Nmu = data.Nmu
            new_data.energy_MC = data.energy_MC
            new_data.electromagnetic_energy = data.electromagnetic_energy
        
        else:
            new_data = data
        
        ###################################
        ## Normalization/Standardization ##
        ###################################
        
        for feature in self.station_features:
            feature_array = new_data.x[:, self.station_features.index(feature)].numpy()
            normalized_array = np.array([self._normalize_feature(value, feature) for value in feature_array])
            new_data.x[:,self.station_features.index(feature)] = torch.tensor(normalized_array)
        
        #######################
        ## edges and weights ##
        #######################
        
        new_data.n_candidates = len(data.station_list)
        # allocate memory for fully connected graph, even if edges end up to be less
        n_stations = len(new_data.station_list)
        edge_index = np.empty((2, n_stations * (n_stations - 1)), dtype=float)
        edge_counter = 0
        top_ = []
        bottom_ = []

        for ids, station in enumerate(new_data.station_list):
            # non-weighted connections in graph based on distance
            for id2 in range(n_stations):
                if ids != id2:
                    dy2 = (float(new_data.pos[ids, 0]) - float(new_data.pos[id2, 0])) ** 2
                    dx2 = (float(new_data.pos[ids, 1]) - float(new_data.pos[id2, 1])) ** 2
                    d = np.sqrt(dx2 + dy2)
                    if (
                        d <= self.dist_neighbour_cut
                    ):  # connect up to 2nd neighbours, some extra distance to account for imperfect array
                        top_.append(ids)
                        bottom_.append(id2)
                        edge_counter += 1

        edge_index = np.vstack((np.array(top_), np.array(bottom_)))

        # keep only assigned edges
        edge_index = edge_index[:, :edge_counter]
        new_data.edge_index = torch.tensor(edge_index, dtype=torch.int16)

        return new_data
        
    def _convolve_trace(self, trace: np.ndarray, a: float, b: float) -> np.ndarray:
        """
        Convolve a single 1D trace with a specified kernel and apply the mathematical formula.
        See GAP-2022-039 from Fiona Ellwanger.

        Args:
            trace (numpy.array): A 1D array (length self.n_time_bins).
            a (float): Parameter `a` for the convolution. Original AoP.
            b (float): Parameter `b` for the convolution. Target AoP.
            
        Returns:
            numpy.array: A 1D array (length self.n_time_bins) after applying the convolution.
        """
        # delta term
        part1 = (a / b) * trace
        step = 1
        # Heaviside is here: t starts from 0
        t = np.arange(0, len(trace), step, dtype=np.float16)
        kernel = ((b - a) / (b ** 2)) * np.exp(-t / b)
        # Normalize the kernel
        integral = (1 - a / b)
        normed_kernel = integral * kernel / (np.sum(kernel) * step)
        part2 = convolve(trace, normed_kernel, mode='same')  # Keep the same length

        result = part1 + part2
        return result
    
    def _apply_convolution_to_traces(self, tensor: torch.Tensor, a: float, b: float) -> torch.Tensor:
        """
        Apply the _convolve_trace function along the second dimension (over each trace).

        Args:
            tensor (torch.Tensor): A 2D tensor of shape (N, 120).
            a (float): Parameter `a` for the convolution.
            b (float): Parameter `b` for the convolution.

        Returns:
            torch.Tensor: A 2D tensor of the same shape (N, 120) after applying the convolution.
        """
        # Apply the function along each row (trace) in the 2D tensor
        result = []
        for trace in tensor:
            # Convert to numpy for _convolve_trace
            trace_np = trace.numpy()
            # Apply the original convolution function
            convolved_trace = self._convolve_trace(trace_np, a, b)
            # Convert back to a PyTorch tensor
            result.append(torch.tensor(convolved_trace, dtype=torch.float32))

        # Stack results back into a single tensor
        return torch.stack(result)
    
    def __get_x_values_from_event(self, idx: int, features: list) -> Dict[str, List[float]]:
        """
        Get x values for specified features from a single event identified by index.

        Args:
            idx (int): The index of the dataset element to retrieve.
            features (list): List of feature names to retrieve from data.x.

        Returns:
            Dict[str, List[float]]: A dictionary where keys are feature names and values are lists of corresponding values.
        """
        data = self.get(idx)  # Retrieve the data for the given index
        values_dict = {}
        
        for feature in features:
            feature_index = self.station_features.index(feature) 
            values_dict[feature] = list(data.x[:,feature_index].numpy())  # Extract the values

        return values_dict

    def compute_normalization_params(self, features: List[str]) -> Dict[str, Dict[str, float]]:
        """
        Compute the mean and standard deviation for specified features across all dataset elements.

        Args:
            features (List[str]): List of feature names to compute normalization parameters for.

        Returns:
            dict: A dictionary containing the mean and std for each feature in the normalization_dict format.
        """

        # Use ThreadPoolExecutor for parallel processing
        with concurrent.futures.ThreadPoolExecutor() as executor:
            # Submit all tasks and gather futures
            futures = [executor.submit(self.__get_x_values_from_event, idx, features) for idx in range(len(self))]

            # Gather results from futures
            all_feature_values = []
            for future in concurrent.futures.as_completed(futures):
                all_feature_values.append(future.result())  # Collect the dictionary from each thread

        # Combine all feature values into a single dictionary
        combined_feature_values = {feature: [] for feature in features}
        for feature_dict in all_feature_values:
            for feature in features:
                combined_feature_values[feature].extend(feature_dict[feature])

        # Calculate mean and std for each feature
        normalization_dict = {}
        for feature in features:
            if combined_feature_values[feature]:
                values = np.array(combined_feature_values[feature])
                normalization_dict[feature] = {
                    'mean': float(np.mean(values)),
                    'std': float(np.std(values)),
                    'method': 'standardization'  # Adjust if needed
                }

        return normalization_dict

##############
## Networks ##
##############

class GNNWithAttentionDiscriminator3Heads(nn.Module):
    def __init__(
        self,
        TA_filters: list,
        TA_kernel_sizes: list,
        TA_strides: list,
        dense_units: list,
        GCN_filters: list,
        num_heads: int,
        sd_node_features: int = 5,
        md_node_features: int = 6,
        sdmd_node_features: int = 7
    ):
        super(GNNWithAttentionDiscriminator3Heads, self).__init__()

        self.TA = TraceAnalyzer(
            filters=TA_filters,
            kernel_sizes=TA_kernel_sizes,
            strides=TA_strides,
        )
        self.sd_node_features = sd_node_features
        self.md_node_features = md_node_features
        self.sdmd_node_features = sdmd_node_features

        ############################
        ## Graph Attention Layers ##
        ############################

        self.sd_attention_convs = nn.ModuleList()
        self.md_attention_convs = nn.ModuleList()
        self.sdmd_attention_convs = nn.ModuleList()

        # Define sd_attention_convs
        for i in range(len(GCN_filters)):
            input_features = sd_node_features + TA_filters[-1] if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.sd_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads,
            ))
            self.sd_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        # Define md_attention_convs
        for i in range(len(GCN_filters)):
            input_features = md_node_features if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.md_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads
            ))
            self.md_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        # Define sdmd_attention_convs
        for i in range(len(GCN_filters)):
            input_features = sdmd_node_features + TA_filters[-1] if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.sdmd_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads
            ))
            self.sdmd_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        ##################
        ## Dense Layers ##
        ##################

        self.sd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
        
        self.sdmd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2 * 3, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )

        self.md_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
        
        # Output layers
        self.output_layers = nn.ModuleList([
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1)
        ])

    def forward(self, inputs) -> torch.Tensor:

        sd_TA_output = self.TA(inputs.x_traces) # traces
        sd_x = torch.cat((inputs.x[:, :self.sd_node_features], sd_TA_output), dim=1) # pure SD
        #sdmd_x = torch.cat((inputs.x, sd_TA_output), dim=1) # SD & MD
        sdmd_x = torch.cat((inputs.x[:, :5], inputs.x[:, 6:], sd_TA_output), dim=1) # SD + MD
        md_x = torch.cat((inputs.x[:, :4], inputs.x[:,6:]), dim=1) # pure MD
        
        for layer in self.sd_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                sd_x = F.relu(layer(sd_x, inputs.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                sd_x = layer(sd_x)
                
        for layer in self.sdmd_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                sdmd_x = F.relu(layer(sdmd_x, inputs.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                sdmd_x = layer(sdmd_x)
                
        for layer in self.md_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                md_x = F.relu(layer(md_x, inputs.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                md_x = layer(md_x)

        # Global pooling
        sd_x = torch.cat([global_mean_pool(sd_x, inputs.batch), global_max_pool(sd_x, inputs.batch)], dim=1)
        sdmd_x = torch.cat([global_mean_pool(sdmd_x, inputs.batch), global_max_pool(sdmd_x, inputs.batch)], dim=1)
        md_x = torch.cat([global_mean_pool(md_x, inputs.batch), global_max_pool(md_x, inputs.batch)], dim=1)
        # we combine everything for the middle head
        sdmd_x_combined = torch.cat([sd_x, sdmd_x, md_x], dim=1)
        
        # Dense Layers
        sd_x = self.sd_dense_layers(sd_x)
        sdmd_x = self.sdmd_dense_layers(sdmd_x_combined)
        md_x = self.md_dense_layers(md_x)

        # Apply output layers
        sd_x = self.output_layers[0](sd_x)
        sdmd_x = self.output_layers[1](sdmd_x)
        md_x = self.output_layers[2](md_x)

        return sd_x, sdmd_x, md_x

    
class GNNWithAttentionDiscriminator3HeadsDualInput(nn.Module):
    def __init__(
        self,
        TA_filters: list,
        TA_kernel_sizes: list,
        TA_strides: list,
        dense_units: list,
        GCN_filters: list,
        num_heads: int,
        sd_node_features: int = 5,
        md_node_features: int = 6,
        sdmd_node_features: int = 8
    ):
        super(GNNWithAttentionDiscriminator3HeadsDualInput, self).__init__()

        self.TA = TraceAnalyzer(
            filters=TA_filters,
            kernel_sizes=TA_kernel_sizes,
            strides=TA_strides,
        )
        self.sd_node_features = sd_node_features
        self.md_node_features = md_node_features
        self.sdmd_node_features = sdmd_node_features

        ############################
        ## Graph Attention Layers ##
        ############################

        self.sd_attention_convs = nn.ModuleList()
        self.md_attention_convs = nn.ModuleList()
        self.sdmd_attention_convs = nn.ModuleList()

        # Define sd_attention_convs
        for i in range(len(GCN_filters)):
            input_features = sd_node_features + TA_filters[-1] if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.sd_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads,
            ))
            self.sd_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        # Define md_attention_convs
        for i in range(len(GCN_filters)):
            input_features = md_node_features if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.md_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads
            ))
            self.md_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        # Define sdmd_attention_convs
        for i in range(len(GCN_filters)):
            input_features = sdmd_node_features + TA_filters[-1] if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.sdmd_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads
            ))
            self.sdmd_attention_convs.append(torch_geometric.nn.norm.BatchNorm(output_features * num_heads, affine=False))

        ##################
        ## Dense Layers ##
        ##################

        self.sd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
        
        self.sdmd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2 * 3, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )

        self.md_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * num_heads * 2, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
        
        # Output layers
        self.output_layers = nn.ModuleList([
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1)
        ])

    def forward(self, inputs) -> torch.Tensor:
        
        sd_graph = inputs[0]
        sdmd_graph = inputs[1]

        sd_TA_output = self.TA(sd_graph.x_traces) # traces
        sdmd_TA_output = self.TA(sdmd_graph.x_traces) # traces
        sd_x = torch.cat((sd_graph.x[:,:self.sd_node_features], sd_TA_output), dim=1) # pure SD
        sdmd_x = torch.cat((sdmd_graph.x[:,:5], sdmd_graph.x[:,6:], sdmd_TA_output), dim=1) # SD & MD # excluding signal
        md_x = torch.cat((sdmd_graph.x[:, :4], sdmd_graph.x[:,6:]), dim=1) # pure MD # excluding SD
        
        for layer in self.sd_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                sd_x = F.relu(layer(sd_x, sd_graph.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                sd_x = layer(sd_x)
                
        for layer in self.sdmd_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                sdmd_x = F.relu(layer(sdmd_x, sdmd_graph.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                sdmd_x = layer(sdmd_x)
                
        for layer in self.md_attention_convs:
            if isinstance(layer, GATv2Conv):
                # Apply specific operation for GATConv layers
                md_x = F.relu(layer(md_x, sdmd_graph.edge_index))
            elif isinstance(layer, torch_geometric.nn.norm.BatchNorm):
                # Apply specific operation for BatchNorm1d layers
                md_x = layer(md_x)

        # Global pooling
        sd_x = torch.cat([global_mean_pool(sd_x, sd_graph.batch), global_max_pool(sd_x, sd_graph.batch)], dim=1)
        sdmd_x = torch.cat([global_mean_pool(sdmd_x, sdmd_graph.batch), global_max_pool(sdmd_x, sdmd_graph.batch)], dim=1)
        md_x = torch.cat([global_mean_pool(md_x, sdmd_graph.batch), global_max_pool(md_x, sdmd_graph.batch)], dim=1)
        # we combine everything for the middle head
        sdmd_x_combined = torch.cat([sd_x, sdmd_x, md_x], dim=1)
        
        # Dense Layers
        sd_x = self.sd_dense_layers(sd_x)
        sdmd_x = self.sdmd_dense_layers(sdmd_x_combined)
        md_x = self.md_dense_layers(md_x)

        # Apply output layers
        sd_x = self.output_layers[0](sd_x)
        sdmd_x = self.output_layers[1](sdmd_x)
        md_x = self.output_layers[2](md_x)

        return sd_x, sdmd_x, md_x
    

class TraceAnalyzer(nn.Module):
    """
    Convolutional model for processing time series features.

    Args:
        filters (List[int]): Number of filters for each convolutional layer.
        kernel_sizes (List[int]): Size of kernels for each convolutional layer.
        strides (List[int]): Stride for each convolutional layer.
        dropout_rate (float): Dropout rate to be applied after each layer except the last one.
    """
    def __init__(self, filters: list, kernel_sizes: list, strides: list):
        super(TraceAnalyzer, self).__init__()
        
        layers = []
        input_channels = 1
        for i in range(len(filters)):
            layers.append(nn.Conv1d(input_channels, filters[i], kernel_sizes[i], stride=strides[i], padding=0))
            layers.append(nn.BatchNorm1d(filters[i], affine=False))
            layers.append(nn.ReLU())
            input_channels = filters[i]
        
        self.conv_layers = nn.Sequential(*layers)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Forward pass of the TraceAnalyzer.

        Args:
            x (torch.Tensor): Input tensor with shape (n_traces, input_size).

        Returns:
            torch.Tensor: Output tensor.
        """
        x = self.conv_layers(x.unsqueeze(1)).squeeze(-1)
        return x
    

class GeometryAnalyzer(nn.Module):
    def __init__(self, GCN_filters: list, num_heads: int, sdmd_node_features: int, TA_filters: list):
        super(GeometryAnalyzer, self).__init__()

        self.sdmd_node_features = sdmd_node_features
        self.sdmd_attention_convs = nn.ModuleList()

        # Define sdmd_attention_convs
        for i in range(len(GCN_filters)):
            input_features = sdmd_node_features + TA_filters[-1] if i == 0 else GCN_filters[i-1] * num_heads
            output_features = GCN_filters[i]
            self.sdmd_attention_convs.append(GATv2Conv(
                input_features, output_features, heads=num_heads
            ))
            self.sdmd_attention_convs.append(BatchNorm(output_features * num_heads, affine=False))

    def forward(self, sdmd_x, edge_index):
        for layer in self.sdmd_attention_convs:
            if isinstance(layer, GATv2Conv):
                sdmd_x = F.relu(layer(sdmd_x, edge_index))
            elif isinstance(layer, BatchNorm):
                sdmd_x = layer(sdmd_x)

        return sdmd_x
    
###########################
## Graph Transformations ##
###########################


class MaskRandomNodes(BaseTransform):
    def __init__(self, max_nodes2prune):
        super(MaskRandomNodes, self).__init__()
        self.max_nodes2prune = max_nodes2prune

    def __call__(self, input_data):
        """
        Prune a random number of nodes from the 1st Crown in the given PyTorch Geometric data object,
        with a maximum specified by max_nodes2prune.

        Args:
            input_data (torch_geometric.data.Data): The data object containing graph information.

        Returns:
            torch_geometric.data.Data: The data object with randomly pruned nodes.
        """
        
        num_nodes = input_data.num_nodes

        # conditions to avoid pruning too much
        if num_nodes <= 4:
            _max_nodes2prune = 0
        elif num_nodes== 5:
            _max_nodes2prune = 1
        else:
            _max_nodes2prune = self.max_nodes2prune
        
        
        data = input_data.clone()

        # we ensure node 0 (hottest station) is not included in the selection
        available2prune = torch.arange(0, num_nodes)
        # Randomly choose the number of nodes to prune based on the max_nodes2prune
        num2prune = torch.randint(0, min(_max_nodes2prune + 1, num_nodes), (1,)).item()
        # Randomly select nodes to be pruned from available nodes
        nodes2prune = available2prune[torch.randperm(len(available2prune))[:num2prune]]
        # Create nodes2keep by excluding nodes to prune from the list of available nodes
        nodes2keep = torch.tensor([node for node in available2prune if node not in nodes2prune])
        # Update the feature matrix, x_traces, total_signal, station_list, and pos to keep only selected nodes
        data.x = data.x[nodes2keep, :]
        data.x_traces = data.x_traces[nodes2keep, :]
        data.station_list = [data.station_list[node] for node in nodes2keep]
        data.pos = data.pos[nodes2keep]

        # Use subgraph method to get the new edge_index
        new_edge_index, _ = subgraph(nodes2keep, edge_index=data.edge_index.long(), relabel_nodes=True, num_nodes=num_nodes)
        data.edge_index = new_edge_index

        # Update the number of nodes in the data object
        data.num_nodes = data.x.size(0)

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
        """
        new_graphs = []
        for graph_batch_id in torch.unique(batch.batch):
            # Get features for this specific graph in the batch
            graph_mask = graph_batch_id == batch.batch
            _graph_x = batch.x[graph_mask]
            _graph_y = batch.y[graph_batch_id]
            _graph_x_traces = batch.x_traces[graph_mask]
            _station_list = batch.station_list[graph_batch_id]
            _seed = batch.seed[graph_batch_id]
            _pos = batch.pos[graph_mask]
            _energy_MC = batch.energy_MC[graph_batch_id]
            _id = batch.id[graph_batch_id] 

            # Get graph edges
            nodes2keep = torch.arange(batch.ptr[graph_batch_id], batch.ptr[graph_batch_id+1])
            _graph_x_edge_index, _ = subgraph(nodes2keep, edge_index=batch.edge_index.long(), relabel_nodes=True, num_nodes=len(batch.batch))

            # Create a new subgraph with the pruned nodes
            new_graph = Data(x=_graph_x, edge_index=_graph_x_edge_index, pos=_pos)
            new_graph.x_traces = _graph_x_traces
            new_graph.station_list = _station_list
            new_graph.y = _graph_y
            new_graph.energy_MC = _energy_MC
            new_graph.seed = _seed
            new_graph.id = _id
            new_graphs.append(new_graph)            

        # Use the Batch class to collate the transformed graphs
        return Batch.from_data_list([self(graph) for graph in new_graphs])
    

class MaskMdCounters(BaseTransform):
    def __init__(self, rho_mu_column_idx, effective_area_column_idx, silent_value: float = 1.0):
        """
        Args:
            rho_mu_column_idx (int): The index of the rho_mu column in data.x.
            effective_area_column_idx (int): The index of the effective_area column in data.x.
        """
        super(MaskMdCounters, self).__init__()
        self.rho_mu_column_idx = rho_mu_column_idx
        self.effective_area_column_idx = effective_area_column_idx
        self.silent_value = silent_value

    def __call__(self, input_data):
        """
        Modify the specified rho_mu and effective_area columns in data.x by randomly setting values to -2 
        in the rho_mu column, and setting values to 0 in the effective_area column for the same rows.

        Args:
            input_data (torch_geometric.data.Data): The data object containing graph information.

        Returns:
            torch_geometric.data.Data: The modified data object.
        """
        data = input_data.clone()  # Clone the input data to avoid modifying the original
        
        # Get the values from the rho_mu column
        rho_mu_values = data.x[:, self.rho_mu_column_idx]

        # Find the indices of rows where the rho_mu value is not equal to the value set to silent stations (-1 if not normalized)
        valid_rows = torch.where(rho_mu_values != self.silent_value)[0]

        # Calculate the number of valid rows
        num_valid_rows = valid_rows.size(0)

        # Randomly determine how many rows to modify, a number between 0 and max(0, num_valid_rows - 3)
        max_rows_to_modify = max(0, num_valid_rows - 3)
        num2modify = torch.randint(0, max_rows_to_modify + 1, (1,)).item()  # Random number between 0 and max_rows_to_modify

        # Randomly choose rows to modify
        if num2modify > 0:
            rows2modify = valid_rows[torch.randperm(num_valid_rows)[:num2modify]]

            # Set the selected rows' rho_mu column values to -2 if not normalized
            data.x[rows2modify, self.rho_mu_column_idx] = 0.0

            # Set the selected rows' effective_area column values to 0
            data.x[rows2modify, self.effective_area_column_idx] = 0

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
        """
        new_graphs = []
        for graph_batch_id in torch.unique(batch.batch):
            # Get features for this specific graph in the batch
            graph_mask = graph_batch_id == batch.batch
            _graph_x = batch.x[graph_mask]
            _graph_y = batch.y[graph_batch_id]
            _graph_x_traces = batch.x_traces[graph_mask]
            _station_list = batch.station_list[graph_batch_id]
            _seed = batch.seed[graph_batch_id]
            _pos = batch.pos[graph_mask]
            _energy_MC = batch.energy_MC[graph_batch_id]
            _id = batch.id[graph_batch_id] 

            # Get graph edges
            nodes2keep = torch.arange(batch.ptr[graph_batch_id], batch.ptr[graph_batch_id+1])
            _graph_x_edge_index, _ = subgraph(nodes2keep, edge_index=batch.edge_index.long(), relabel_nodes=True, num_nodes=len(batch.batch))

            # Create a new subgraph with the pruned nodes
            new_graph = Data(x=_graph_x, edge_index=_graph_x_edge_index, pos=_pos)
            new_graph.x_traces = _graph_x_traces
            new_graph.station_list = _station_list
            new_graph.y = _graph_y
            new_graph.energy_MC = _energy_MC
            new_graph.seed = _seed
            new_graph.id = _id
            new_graphs.append(new_graph)            

        # Use the Batch class to collate the transformed graphs
        return Batch.from_data_list([self(graph) for graph in new_graphs])
    

class MaskNodesSD433(BaseTransform):
    def __init__(self, max_nodes2prune=15):
        super(MaskNodeSD433, self).__init__()
        self.max_nodes2prune = max_nodes2prune

        # Hardcoded list of station IDs that we might want to mask
        self.stations_to_mask = np.array([# SD-433
                            #central station
                            1764,
                            # 1st crown
                            97, 13, 47, 99, 11, 98, 30, 12,
                            # 2nd crown
                            28, 1769, 54, 1765, 50, 734, 42, 1773, 27, 1622, 29, 688])

    def __call__(self, input_data):
        """
        Prune a random number of nodes from the graph based on the hardcoded station IDs.
        Exclude the first station (index 0) and any stations in the seed list.

        Args:
            input_data (torch_geometric.data.Data): The data object containing graph information.
        
        Returns:
            torch_geometric.data.Data: The data object with randomly pruned nodes.
        """
        data = input_data.clone()  # Clone the input data to avoid modifying the original

        # Get the number of nodes
        num_nodes = data.num_nodes
        all_nodes = torch.arange(0, num_nodes)

        # Create an exclusion list that includes index 0 (first station) and the indices corresponding to station IDs in data.seed
        exclusion_list = [0]  # Always exclude the first station (index 0)

        # Map station IDs in data.seed to their corresponding indices in station_list
        for seed_id in data.seed:
            try:
                # Find the index of the seed station ID in station_list
                seed_index = data.station_list.index(seed_id)
                exclusion_list.append(seed_index)
            except ValueError:
                # If the seed ID is not in the station list, skip it
                pass

        # Create a list of nodes that can potentially be pruned (from the predefined list of station IDs)
        nodes_to_prune = []
        for station_id in self.stations_to_mask:
            try:
                # Find the index of each station ID in station_list
                station_index = data.station_list.index(station_id)
                if station_index not in exclusion_list:
                    nodes_to_prune.append(station_index)
            except ValueError:
                # If the station ID is not in the station list, skip it
                pass

        nodes_to_prune = torch.tensor(nodes_to_prune)

        # Determine the number of nodes to prune, randomly selected between 0 and max_nodes2prune
        num2prune = torch.randint(0, min(self.max_nodes2prune + 1, len(nodes_to_prune) + 1), (1,)).item()

        # Randomly select nodes to prune from the available nodes
        if num2prune > 0:
            nodes2prune = nodes_to_prune[torch.randperm(len(nodes_to_prune))[:num2prune]]
        else:
            nodes2prune = torch.tensor([])  # No nodes to prune

        # Create nodes2keep by excluding nodes to prune from the list of all nodes
        nodes2keep = torch.tensor([node for node in all_nodes if node not in nodes2prune])

        # Update the feature matrix (x), traces (x_traces), position (pos), and station_list to only keep selected nodes
        data.x = data.x[nodes2keep, :]
        data.x_traces = data.x_traces[nodes2keep, :]
        data.station_list = [data.station_list[node] for node in nodes2keep]
        data.pos = data.pos[nodes2keep]

        # Use subgraph method to update the edge_index and relabel the nodes
        new_edge_index, _ = subgraph(nodes2keep, edge_index=data.edge_index.long(), relabel_nodes=True, num_nodes=num_nodes)
        data.edge_index = new_edge_index

        # Update the number of nodes in the data object
        data.num_nodes = data.x.size(0)

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
        """
        new_graphs = []
        for graph_batch_id in torch.unique(batch.batch):
            # Get features for this specific graph in the batch
            graph_mask = graph_batch_id == batch.batch
            _graph_x = batch.x[graph_mask]
            _graph_y = batch.y[graph_batch_id]
            _graph_x_traces = batch.x_traces[graph_mask]
            _station_list = batch.station_list[graph_batch_id]
            _seed = batch.seed[graph_batch_id]
            _pos = batch.pos[graph_mask]
            _energy_MC = batch.energy_MC[graph_batch_id]
            _id = batch.id[graph_batch_id]

            # Get graph edges
            nodes2keep = torch.arange(batch.ptr[graph_batch_id], batch.ptr[graph_batch_id+1])
            _graph_x_edge_index, _ = subgraph(nodes2keep, edge_index=batch.edge_index.long(), relabel_nodes=True, num_nodes=len(batch.batch))

            # Create a new subgraph with the pruned nodes
            new_graph = Data(x=_graph_x, edge_index=_graph_x_edge_index, pos=_pos)
            new_graph.x_traces = _graph_x_traces
            new_graph.station_list = _station_list
            new_graph.y = _graph_y
            new_graph.energy_MC = _energy_MC
            new_graph.seed = _seed
            new_graph.id = _id
            new_graphs.append(new_graph)            

        # Use the Batch class to collate the transformed graphs
        return Batch.from_data_list([self(graph) for graph in new_graphs])
    

class MaskNodes(BaseTransform):
    def __init__(self, max_nodes2prune=20):
        super(MaskNodes, self).__init__()
        self.max_nodes2prune = max_nodes2prune

    def __call__(self, input_data):
        """
        Prune a random number of nodes from the graph, excluding the first station (index 0)
        and any stations in the seed list (which are represented by station IDs in station_list).
        
        Args:
            input_data (torch_geometric.data.Data): The data object containing graph information.
        
        Returns:
            torch_geometric.data.Data: The data object with randomly pruned nodes.
        """
        data = input_data.clone()  # Clone the input data to avoid modifying the original

        # Get the number of nodes
        num_nodes = data.num_nodes
        all_nodes = torch.arange(0, num_nodes)

        # Create an exclusion list that includes index 0 (first station) and the indices corresponding to station IDs in data.seed
        exclusion_list = [0]  # Always exclude the first station (index 0)

        for seed_id in data.seed:
            try:
                # Find the index of the seed station ID in station_list
                seed_index = data.station_list.index(seed_id)
                exclusion_list.append(seed_index)
            except ValueError:
                # If the seed ID is not in the station list, skip it
                pass
            
        exclusion_list = list(set(sorted(exclusion_list)))
        
        # Create a list of nodes that can be pruned (exclude the ones in exclusion_list)
        available2prune = torch.tensor([node for node in all_nodes if node not in exclusion_list])

        # Determine the number of nodes to prune, randomly selected between 0 and max_nodes2prune
        num2prune = torch.randint(0, min(self.max_nodes2prune + 1, available2prune.size(0) + 1), (1,)).item()

        # Randomly select nodes to prune from the available nodes
        nodes2prune = available2prune[torch.randperm(len(available2prune))[:num2prune]]
        
        # Create nodes2keep by excluding nodes to prune from the list of all nodes
        nodes2keep = torch.tensor([node for node in all_nodes if node not in nodes2prune])

        # Update the feature matrix (x), traces (x_traces), position (pos), and station_list to only keep selected nodes
        data.x = data.x[nodes2keep, :]
        data.x_traces = data.x_traces[nodes2keep, :]
        data.station_list = [data.station_list[node] for node in nodes2keep]
        data.pos = data.pos[nodes2keep]

        # Use subgraph method to update the edge_index and relabel the nodes
        new_edge_index, _ = subgraph(nodes2keep, edge_index=data.edge_index.long(), relabel_nodes=True, num_nodes=num_nodes)
        data.edge_index = new_edge_index

        # Update the number of nodes in the data object
        data.num_nodes = data.x.size(0)

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
        """
        new_graphs = []
        for graph_batch_id in torch.unique(batch.batch):
            # Get features for this specific graph in the batch
            graph_mask = graph_batch_id == batch.batch
            _graph_x = batch.x[graph_mask]
            _graph_y = batch.y[graph_batch_id]
            _graph_x_traces = batch.x_traces[graph_mask]
            _station_list = batch.station_list[graph_batch_id]
            _seed = batch.seed[graph_batch_id]
            _pos = batch.pos[graph_mask]
            _energy_MC = batch.energy_MC[graph_batch_id]
            _id = batch.id[graph_batch_id]

            # Get graph edges
            nodes2keep = torch.arange(batch.ptr[graph_batch_id], batch.ptr[graph_batch_id+1])
            _graph_x_edge_index, _ = subgraph(nodes2keep, edge_index=batch.edge_index.long(), relabel_nodes=True, num_nodes=len(batch.batch))

            # Create a new subgraph with the pruned nodes
            new_graph = Data(x=_graph_x, edge_index=_graph_x_edge_index, pos=_pos)
            new_graph.x_traces = _graph_x_traces
            new_graph.station_list = _station_list
            new_graph.y = _graph_y
            new_graph.energy_MC = _energy_MC
            new_graph.seed = _seed
            new_graph.id = _id
            new_graphs.append(new_graph)            

        # Use the Batch class to collate the transformed graphs
        return Batch.from_data_list([self(graph) for graph in new_graphs])

    
class SilentPrunner(BaseTransform):
    def __init__(self, silent_col_index: int = 4, silent_value: float = -0.5):
        """
        Args:
            silent_col_index (int): The index of the column to check for "silent" values.
            silent_value (float): The value used to select silent stations.
        """
        super(SilentPrunner, self).__init__()
        self.silent_col_index = silent_col_index
        self.silent_value = silent_value

    def __call__(self, input_data):
        """
        Modify the specified rho_mu and effective_area columns in data.x by randomly setting values to -2 
        in the rho_mu column, and setting values to 0 in the effective_area column for the same rows.

        Args:
            input_data (torch_geometric.data.Data): The data object containing graph information.

        Returns:
            torch_geometric.data.Data: The modified data object.
        """
        data = input_data.clone()  # Clone the input data to avoid modifying the original
        num_nodes = data.num_nodes
        
        # Get the values from the rho_mu column
        potential_silent_values = data.x[:, self.silent_col_index]

        # Find the indices of rows where the rho_mu value is not equal to the value set to silent stations (-1 if not normalized)
        nodes2keep = torch.where(potential_silent_values != self.silent_value)[0]

        # Update the feature matrix (x), traces (x_traces), position (pos), and station_list to only keep selected nodes
        data.x = data.x[nodes2keep, :]
        data.x_traces = data.x_traces[nodes2keep, :]
        data.station_list = [data.station_list[node] for node in nodes2keep]
        data.pos = data.pos[nodes2keep]

        # Use subgraph method to update the edge_index and relabel the nodes
        new_edge_index, _ = subgraph(nodes2keep, edge_index=data.edge_index.long(), relabel_nodes=True, num_nodes=num_nodes)
        data.edge_index = new_edge_index

        # Update the number of nodes in the data object
        data.num_nodes = data.x.size(0)

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
        """
        new_graphs = []
        for graph_batch_id in torch.unique(batch.batch):
            # Get features for this specific graph in the batch
            graph_mask = graph_batch_id == batch.batch
            _graph_x = batch.x[graph_mask]
            _graph_y = batch.y[graph_batch_id]
            _graph_x_traces = batch.x_traces[graph_mask]
            _station_list = batch.station_list[graph_batch_id]
            _seed = batch.seed[graph_batch_id]
            _pos = batch.pos[graph_mask]
            _energy_MC = batch.energy_MC[graph_batch_id]
            _id = batch.id[graph_batch_id] 

            # Get graph edges
            nodes2keep = torch.arange(batch.ptr[graph_batch_id], batch.ptr[graph_batch_id+1])
            _graph_x_edge_index, _ = subgraph(nodes2keep, edge_index=batch.edge_index.long(), relabel_nodes=True, num_nodes=len(batch.batch))

            # Create a new subgraph with the pruned nodes
            new_graph = Data(x=_graph_x, edge_index=_graph_x_edge_index, pos=_pos)
            new_graph.x_traces = _graph_x_traces
            new_graph.station_list = _station_list
            new_graph.y = _graph_y
            new_graph.energy_MC = _energy_MC
            new_graph.seed = _seed
            new_graph.id = _id
            new_graphs.append(new_graph)            

        # Use the Batch class to collate the transformed graphs
        return Batch.from_data_list([self(graph) for graph in new_graphs])

class SD433UMDatasetHeterogeneous(SD433UMDatasetPyG):
    """
    Child class of SD433UMDatasetPyG that implements heterogeneous graph construction.

    Features are divided among three node types:
      - Spatio-Temporal (ST)
      - Signal Detector (SD)
      - Muon Density (MD)

    Edges are undirected and:
      - Connect ST nodes based on dist_neighbour_cut.
      - Link each MD and SD node to its respective ST node.
    """

    def __init__(self, file_paths: List[str], root: str, n_time_bins: int = 120,
                 augmentation_options: dict = None, normalization_dict: dict = None, dist_neighbour_cut: float = 790.0, node_feature_map: dict = None):
        super().__init__(file_paths, root, normalization_dict)
        self.augmentation_options = augmentation_options or {}
        self.n_time_bins = n_time_bins
        self.dist_neighbour_cut = dist_neighbour_cut
        self.node_feature_map = node_feature_map or {}

    def get(self, idx) -> HeteroData:
        """
        Retrieve the heterogeneous graph data at the specified index.

        Args:
            idx (int): Index of the data.

        Returns:
            HeteroData: The processed heterogeneous graph data.
        """
        data = torch.load(os.path.join(self.processed_dir, f"{self.id_list[idx]}.pt"))
        # set the lenght of traces to use
        data.x_traces = data.x_traces[:, :, :self.n_time_bins]
        hetero_data = HeteroData()

        #############################
        ## Node Feature Assignment ##
        #############################

        # Spatio-Temporal (ST) Nodes
        hetero_data["ST"].x = data.x[:, :4]

        
        ##################
        ## Augmentation ##
        ##################
        
        # SD part - Check if 'mask_PMTs' augmentation is enabled
        if self.augmentation_options.get('mask_PMTs', False):
            # Loop through stations
            for idx in range(len(data.station_list)):
                n_PMTs = torch.randint(1, 4, (1,))  # Uniform sampling to be aggressive!
                selected_indexes = torch.randperm(3)[:n_PMTs]
        
                # Create a mask for the selected PMTs
                mask = torch.zeros(3, dtype=torch.bool)
                mask[selected_indexes] = True
        
                # Set non-selected traces and signals to zero
                masked_traces = data.x_traces[idx]
                masked_traces[~mask, :] = 0
        
                # Update the original data
                data.x_traces[idx] = masked_traces
        
        # AoP Transformation and Dynamic-Range limitation (fixed saturation threshold)
        if self.augmentation_options.get('AoP_and_saturation', False):
            meanAoP_from_data, stdAoP_from_data = 2.92, 0.2
            AoP_from_sims = 3.2
        
            # Generate a random augmented AoP for each PMT in each station
            augmented_AoPs = np.random.normal(loc=meanAoP_from_data, scale=stdAoP_from_data, size=(data.x_traces.shape[0], 3))
        
            # Apply convolution with unique augmented AoP per PMT, checking for zero traces
            data.x_traces, data.AoP = self._apply_random_convolution_to_traces(data.x_traces, AoP_from_sims, augmented_AoPs)
            data.x_traces = torch.clamp(data.x_traces, max=120.0, min=-0.99)  # saturation threshold in VEM charge decided based on data
        
        # trace log normalization (always applied unless specified otherwise in options)
        if self.augmentation_options.get('log_normalize_traces', False):
            data.x_traces = torch.log10(data.x_traces + 1) / np.log10(101)
        
        # MD part - Check if 'mask_MD_mods' augmentation is enabled
        if self.augmentation_options.get('mask_MD_mods', False):
            # Loop through stations
            for idx in range(len(data.station_list)):
                n_modules = torch.randint(1, 4, (1,))  # Uniform sampling to be aggressive!
                selected_indexes = torch.randperm(3)[:n_modules]
        
                # Create a mask for the selected modules
                mask = torch.zeros(3, dtype=torch.bool)
                mask[selected_indexes] = True
        
                # Set non-selected effective areas and Nmu to zero
                data.effective_area[idx, ~mask] = 0
                data.md_Nmu[idx, ~mask] = 0

        ##################################################
        ## Edge Construction  and PMT/module assignment ##
        ##################################################

        # Edges between ST nodes based on distance
        n_st_nodes = hetero_data["ST"].x.shape[0]
        edge_index_st = []
        for i in range(n_st_nodes):
            for j in range(n_st_nodes):
                if i != j:
                    dx = data.pos[i, 0] - data.pos[j, 0]
                    dy = data.pos[i, 1] - data.pos[j, 1]
                    distance = (dx**2 + dy**2)**0.5
                    if distance <= self.dist_neighbour_cut:
                        edge_index_st.append((i, j))
                        edge_index_st.append((j, i))  # Add the reverse edge
        
        # Convert to tensor and assign
        hetero_data["ST", "is_neighbour", "ST"].edge_index = torch.tensor(edge_index_st, dtype=torch.long).t().contiguous()

        # Assign integer IDs to SD_trace and SD_scalar nodes and MD modules
        sd_trace_id = 0
        sd_scalar_id = 0
        md_module_id = 0
    
        # Initialize node feature matrices and edge indices if not already present
        if "SD_trace" not in hetero_data:
            hetero_data["SD_trace"].x = torch.empty((0, data.x_traces.shape[2]), dtype=torch.float32)  # Match trace feature dimensions
            hetero_data["ST", "has_SD_trace", "SD_trace"].edge_index = torch.empty((2, 0), dtype=torch.long)
        
        if "SD_scalar" not in hetero_data:
            hetero_data["SD_scalar"].x = torch.empty((0, 1), dtype=torch.float32)  # Scalar values are 1D
            hetero_data["ST", "has_SD_scalar", "SD_scalar"].edge_index = torch.empty((2, 0), dtype=torch.long)
        
        if "MD" not in hetero_data:
            hetero_data["MD"].x = torch.empty((0, 2), dtype=torch.float32)  # Effective area and density (2D features)
            hetero_data["ST", "has_MD", "MD"].edge_index = torch.empty((2, 0), dtype=torch.long)
        
        # Create SD_trace and SD_scalar nodes
        for station_idx, station_traces in enumerate(data.x_traces):
            for pmt_idx, trace in enumerate(station_traces):
                if (not torch.all(trace == 0)) and (data.AoP[station_idx, pmt_idx] != 0):
                    # Add SD_trace node
                    hetero_data["SD_trace"].x = torch.cat((hetero_data["SD_trace"].x, trace.unsqueeze(0)))
                    hetero_data["ST", "has_SD_trace", "SD_trace"].edge_index = torch.cat(
                        (
                            hetero_data["ST", "has_SD_trace", "SD_trace"].edge_index,
                            torch.tensor([[station_idx], [sd_trace_id]], dtype=torch.long),
                        ),
                        dim=1,
                    )
        
                    # Add SD_scalar node
                    scalar_value = torch.tensor([[data.AoP[station_idx, pmt_idx]]])  # 2D tensor for concatenation
                    hetero_data["SD_scalar"].x = torch.cat((hetero_data["SD_scalar"].x, scalar_value))
                    hetero_data["ST", "has_SD_scalar", "SD_scalar"].edge_index = torch.cat(
                        (
                            hetero_data["ST", "has_SD_scalar", "SD_scalar"].edge_index,
                            torch.tensor([[station_idx], [sd_scalar_id]], dtype=torch.long),
                        ),
                        dim=1,
                    )
        
                    sd_trace_id += 1
                    sd_scalar_id += 1
        
        # Create MD nodes
        for station_idx, (eff_area, nmu) in enumerate(zip(data.effective_area, np.log10(data.md_Nmu+1))): # using the log of density
            for mod_idx, (area, mu) in enumerate(zip(eff_area, nmu)):
                if area != 0:
                    # Add MD node
                    md_features = torch.tensor([[area, mu]])  # 2D tensor for concatenation
                    hetero_data["MD"].x = torch.cat((hetero_data["MD"].x, md_features))
                    hetero_data["ST", "has_MD", "MD"].edge_index = torch.cat(
                        (
                            hetero_data["ST", "has_MD", "MD"].edge_index,
                            torch.tensor([[station_idx], [md_module_id]], dtype=torch.long),
                        ),
                        dim=1,
                    )
        
                    md_module_id += 1
        
        ###################################
        ## Normalization/Standardization ##
        ###################################
        
        for feature in self.normalization_dict.keys():
            # Determine the node type and column index for the feature using node_feature_map
            for node_type, features in self.node_feature_map.items():
                if feature in features:
                    
                    feature_index = features.index(feature)  # Get the column index for the feature
        
                    # Access the correct feature matrix for the node type
                    feature_matrix = hetero_data[node_type].x  # Use the node type to get the correct matrix
        
                    # Extract the column to normalize
                    feature_array = feature_matrix[:, feature_index].numpy()
        
                    # Normalize using _normalize_feature
                    normalized_array = np.array([self._normalize_feature(value, feature) for value in feature_array])
        
                    # Update the feature matrix with the normalized values
                    hetero_data[node_type].x[:, feature_index] = torch.tensor(normalized_array, dtype=torch.float32)
        
        ###################
        ## Assign Labels ##
        ###################
        hetero_data.station_list = data.station_list
        hetero_data.id = data.id
        hetero_data.y = data.y
        hetero_data.pos = data.pos # SD stations
        hetero_data.core = data.core
        hetero_data.seed = [(station_id - 4000) if (station_id>4000) else station_id for station_id in data.seed]
        hetero_data.Xmax = data.Xmax
        hetero_data.Nmu = data.Nmu
        hetero_data.energy_MC = data.energy_MC
        hetero_data.electromagnetic_energy = data.electromagnetic_energy
        
        return hetero_data

    def _convolve_trace(self, trace: np.ndarray, a: float, b: float) -> np.ndarray:
        """
        Convolve a single 1D trace with a specified kernel and apply the mathematical formula.
        See GAP-2022-039 from Fiona Ellwanger.

        Args:
            trace (numpy.array): A 1D array (length self.n_time_bins).
            a (float): Parameter `a` for the convolution. Original AoP.
            b (float): Parameter `b` for the convolution. Target AoP.
            
        Returns:
            numpy.array: A 1D array (length self.n_time_bins) after applying the convolution.
        """
        # delta term
        part1 = (a / b) * trace
        step = 1
        # Heaviside is here: t starts from 0
        t = np.arange(0, len(trace), step, dtype=np.float16)
        kernel = ((b - a) / (b ** 2)) * np.exp(-t / b)
        # Normalize the kernel
        integral = (1 - a / b)
        normed_kernel = integral * kernel / (np.sum(kernel) * step)
        part2 = convolve(trace, normed_kernel, mode='same')  # Keep the same length

        result = part1 + part2
        return result
        
    def _apply_random_convolution_to_traces(self, tensor: torch.Tensor, a: float, augmented_AoPs: np.ndarray) -> (torch.Tensor, torch.Tensor):
        """
        Apply the _convolve_trace function with a unique augmented AoP to each PMT, checking for zero traces.
    
        Args:
            tensor (torch.Tensor): A 3D tensor of shape (N, 3, time_bins).
            a (float): Parameter `a` for the convolution.
            augmented_AoPs (np.ndarray): A 2D array of shape (N, 3) containing the `b` parameter for each PMT.
    
        Returns:
            tuple: A tuple containing:
                - torch.Tensor: A 3D tensor of the same shape after applying the convolution.
                - torch.Tensor: A 2D tensor of shape (N, 3) storing the AoP values for each PMT.
        """
        result_traces = []
        result_AoPs = []
    
        for idx, station_traces in enumerate(tensor):
            station_result_traces = []
            station_result_AoPs = []
    
            for pmt_idx, trace in enumerate(station_traces):
                if torch.all(trace == 0):  # Check if the trace is all zeros
                    station_result_traces.append(trace)
                    station_result_AoPs.append(0.0)
                else:
                    # Convert to numpy for _convolve_trace
                    trace_np = trace.numpy()
                    # Apply the original convolution function with a unique augmented AoP
                    convolved_trace = self._convolve_trace(trace_np, a, augmented_AoPs[idx, pmt_idx])
                    # Convert back to a PyTorch tensor
                    station_result_traces.append(torch.tensor(convolved_trace, dtype=torch.float32))
                    station_result_AoPs.append(augmented_AoPs[idx, pmt_idx])
    
            result_traces.append(torch.stack(station_result_traces))
            result_AoPs.append(torch.tensor(station_result_AoPs, dtype=torch.float32))
    
        # Stack results back into tensors
        return torch.stack(result_traces), torch.stack(result_AoPs)


    def __get_x_values_from_event(self, idx: int, node_features: Dict[str, List[str]]) -> Dict[str, List[float]]:
        """
        Extract feature values for specified node types and features from a single event identified by index.
    
        Args:
            idx (int): The index of the dataset element to retrieve.
            node_features (dict): Dictionary where keys are node types and values are lists of features to retrieve.
    
        Returns:
            Dict[str, List[float]]: A dictionary where keys are feature names and values are lists of feature values.
        """
        data = self.get(idx)
        values_dict = {}
    
        for node_type, features in node_features.items():
            if node_type in data.node_types:
                for feature in features:
                    # Map feature name to index using node_feature_map
                    feature_index = self.node_feature_map[node_type].index(feature)
                    # Save feature values with the feature name as the key
                    values_dict[feature] = data[node_type].x[:, feature_index].tolist()
    
        return values_dict


    def compute_normalization_params(self, node_features: Dict[str, List[str]]) -> Dict[str, Dict[str, float]]:
        """
        Compute mean and std for specified features across all node types in the dataset.
    
        Args:
            node_features (dict): Dictionary where keys are node types and values are lists of features.
    
        Returns:
            dict: A dictionary where keys are feature names, and values are their normalization parameters.
        """
        # Use ThreadPoolExecutor for parallel processing
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = [executor.submit(self.__get_x_values_from_event, idx, node_features) for idx in range(len(self))]
    
            # Gather results from futures
            all_feature_values = []
            for future in concurrent.futures.as_completed(futures):
                all_feature_values.append(future.result())
    
        # Combine all feature values into a single dictionary keyed by feature
        combined_feature_values = {feature: [] for node_features_list in node_features.values() for feature in node_features_list}
    
        for feature_dict in all_feature_values:
            for feature, values in feature_dict.items():
                combined_feature_values[feature].extend(values)
    
        # Calculate mean and std for each feature
        normalization_dict = {}
        for feature, values in combined_feature_values.items():
            if values:
                values_array = np.array(values)
                normalization_dict[feature] = {
                    'mean': float(np.mean(values_array)),
                    'std': float(np.std(values_array)),
                    'method': 'standardization'
                }
    
        return normalization_dict

class HeteroGNNWith3Heads(nn.Module):
    def __init__(
        self,
        TA_filters: list,
        TA_kernel_sizes: list,
        TA_strides: list,
        dense_units: list,
        GCN_filters: list,
        num_heads: int,
        in_channels: dict,  # Pass a dictionary with input feature sizes for each node type
    ):
        super(HeteroGNNWith3Heads, self).__init__()

        # Trace Analyzer for "SD_trace"
        self.TA = TraceAnalyzer(
            filters=TA_filters,
            kernel_sizes=TA_kernel_sizes,
            strides=TA_strides,
        )

        # Define HAN Layers for each path with HeteroBatchNorm
        self.han_SD = nn.ModuleList()
        self.han_SDMD = nn.ModuleList()
        self.han_MD = nn.ModuleList()

        # Initialization of the SD-only HAN layers
        for i in range(len(GCN_filters)):
            if i == 0:
                self.han_SD.append(nn.ModuleList([
                    HANConv(
                        in_channels={"ST": in_channels["ST"], "SD": in_channels["SD"]},  # Initial input sizes
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "SD"], [("ST", "is_neighbour", "ST"), ("ST", "has_SD", "SD")]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=2)  # Two node types: ST and SD
                ]))
            else:
                self.han_SD.append(nn.ModuleList([
                    HANConv(
                        in_channels={"ST": GCN_filters[i - 1], "SD": GCN_filters[i - 1]},
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "SD"], [("ST", "is_neighbour", "ST"), ("ST", "has_SD", "SD")]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=2)
                ]))
        
        # Initialization of the Full graph HAN layers (ST, SD, MD nodes)
        for i in range(len(GCN_filters)):
            if i == 0:
                self.han_SDMD.append(nn.ModuleList([
                    HANConv(
                        in_channels={"ST": in_channels["ST"], "SD": in_channels["SD"], "MD": in_channels["MD"]},  # Initial sizes
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "SD", "MD"], [
                            ("ST", "is_neighbour", "ST"),
                            ("ST", "has_SD", "SD"),
                            ("ST", "has_MD", "MD")
                        ]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=3)  # Three node types: ST, SD, and MD
                ]))
            else:
                self.han_SDMD.append(nn.ModuleList([
                    HANConv(
                        in_channels={
                            "ST": GCN_filters[i - 1],
                            "SD": GCN_filters[i - 1],
                            "MD": GCN_filters[i - 1]
                        },
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "SD", "MD"], [
                            ("ST", "is_neighbour", "ST"),
                            ("ST", "has_SD", "SD"),
                            ("ST", "has_MD", "MD")
                        ]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=3)
                ]))
        
        # Initialization of the MD-only HAN layers
        for i in range(len(GCN_filters)):
            if i == 0:
                self.han_MD.append(nn.ModuleList([
                    HANConv(
                        in_channels={"ST": in_channels["ST"], "MD": in_channels["MD"]},  # Initial sizes
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "MD"], [("ST", "is_neighbour", "ST"), ("ST", "has_MD", "MD")]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=2)  # Two node types: ST and MD
                ]))
            else:
                self.han_MD.append(nn.ModuleList([
                    HANConv(
                        in_channels={"ST": GCN_filters[i - 1], "MD": GCN_filters[i - 1]},
                        out_channels=GCN_filters[i],
                        metadata=(["ST", "MD"], [("ST", "is_neighbour", "ST"), ("ST", "has_MD", "MD")]),
                        heads=num_heads
                    ),
                    HeteroBatchNorm(GCN_filters[i], num_types=2)
                ]))

        ##################
        ## Dense Layers ##
        ##################
        sd_edge_types = 2 #(ST-ST + ST-SD)
        sdmd_edge_types = 3 #(ST-ST + ST-SD + ST+MD)
        md_edge_types = 2 #(ST-ST + ST+MD)
        
        self.sd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * 2 * sd_edge_types, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )


        self.sdmd_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * 2 * (sd_edge_types + sdmd_edge_types + md_edge_types), dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
 
        self.md_dense_layers = nn.Sequential(
            nn.Linear(GCN_filters[-1] * 2 * md_edge_types, dense_units[0]),
            nn.BatchNorm1d(dense_units[0], affine=False),
            nn.ReLU(),
            nn.Linear(dense_units[0], dense_units[1]),
            nn.BatchNorm1d(dense_units[1], affine=False),
            nn.ReLU(),
        )
        
        # Output layers
        self.output_layers = nn.ModuleList([
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1),
            nn.Linear(dense_units[1], 1)
        ])

    def forward(self, data: HeteroData, return_attention_coefficients=False, debug=False) -> torch.Tensor:
        attention_coefficients = {"SD": None, "SDMD": None, "MD": None}

        # Process SD_trace nodes with Trace Analyzer
        TA_output = self.TA(data["SD_trace"].x)  # Process SD_trace features
        SD_features = torch.cat([TA_output, data["SD_scalar"].x], dim=1)  # Concatenate with SD_scalar

        # trick to keep batch info when creating a new node and deleting old
        my_batch_dict = {}
        my_batch_dict["ST"] = data.batch_dict["ST"]
        my_batch_dict["MD"] = data.batch_dict["MD"]
        my_batch_dict["SD"] = data.batch_dict["SD_scalar"]
        
        # Replace SD_trace and SD_scalar nodes with unified SD node
        data["SD"].x = SD_features
        data["ST", "has_SD", "SD"].edge_index = data["ST", "has_SD_scalar", "SD_scalar"].edge_index  # Reassign edge index before deletion
        data.batch_dict["SD"] = data.batch_dict["SD_scalar"] 
        
        # Safely delete old node types and edges
        del data["SD_scalar"], data["SD_trace"]
        del data["ST", "has_SD_scalar", "SD_scalar"], data["ST", "has_SD_trace", "SD_trace"]

        # Create copies of the original data for each path
        data_sd = data.clone()  # For the 1st head (ST and SD nodes only)
        data_md = data.clone()  # For the 3rd head (ST and MD nodes only)

        #################################
        # Prune MD nodes from `data_sd` #
        #################################
        del data_sd["MD"], data_sd["ST", "has_MD", "MD"]

        #################################
        # Prune SD nodes from `data_md` #
        #################################
        del data_md["SD"], data_md["ST", "has_SD", "SD"]

        def process_path(x_dict, edge_index_dict, batch_dict, layers, debug_label):
            if debug:
                print(f"Processing {debug_label}")
                print("Initial x_dict shapes:")
                for node_type, features in x_dict.items():
                    print(f"  {node_type}: {features.shape if features is not None else 'None'}")
                print("Initial edge_index_dict shapes:")
                for edge_type, edge_index in edge_index_dict.items():
                    print(f"  {edge_type}: {edge_index.shape if edge_index is not None else 'None'}")

            for idx, layer_group in enumerate(layers):
                if debug:
                    print(f"Applying layer group {idx + 1} ({debug_label})")
                for layer in layer_group:
                    if isinstance(layer, HANConv):
                        if idx == 0 and return_attention_coefficients and debug_label == "Path 1":
                            x_dict, attn = layer(x_dict, edge_index_dict, return_attention_coefficients=True)
                            attention_coefficients[debug_label.split()[1]] = attn  # Store attention coefficients
                        else:
                            x_dict = layer(x_dict, edge_index_dict)
                        if debug:
                            print("x_dict shapes after HANConv:")
                            for node_type, features in x_dict.items():
                                print(f"  {node_type}: {features.shape if features is not None else 'None'}")
                            print("Edge shapes after HANConv:")
                            for edge_type, edge_index in edge_index_dict.items():
                                print(f"  {edge_type}: {edge_index.shape if edge_index is not None else 'None'}")
                    elif isinstance(layer, HeteroBatchNorm):
                        x_dict = {
                            node_type: layer(
                                x=features,
                                type_vec=torch.full((features.size(0),), idx, dtype=torch.long, device=features.device)
                            )
                            for idx, (node_type, features) in enumerate(x_dict.items())
                            if features is not None
                        }
                        if debug:
                            print("x_dict shapes after BatchNorm:")
                            for node_type, features in x_dict.items():
                                print(f"  {node_type}: {features.shape if features is not None else 'None'}")
            return torch.cat([
                hetero_global_pool(x_dict, batch_dict, pool_type="mean"),
                hetero_global_pool(x_dict, batch_dict, pool_type="max")
            ], dim=1)

        # Path 1: SD-only graph (ST and SD nodes)
        sd_x = process_path(data_sd.x_dict, data_sd.edge_index_dict, my_batch_dict, self.han_SD, "Path 1: SD-only graph")

        # Path 2: Full graph (ST, SD, MD nodes)
        sdmd_x = process_path(data.x_dict, data.edge_index_dict, my_batch_dict, self.han_SDMD, "Path 2: Full graph")

        # Path 3: MD-only graph (ST and MD nodes)
        md_x = process_path(data_md.x_dict, data_md.edge_index_dict, my_batch_dict, self.han_MD, "Path 3: MD-only graph")

        if debug:
            print(f"sd_x shape after pooling: {sd_x.shape}")
            print(f"sdmd_x shape after pooling: {sdmd_x.shape}")
            print(f"md_x shape after pooling: {md_x.shape}")

        #################################
        # Combine paths for middle head #
        #################################
        sdmd_x_combined = torch.cat([sd_x, sdmd_x, md_x], dim=1)

        ################
        # Dense Layers #
        ################
        sd_x = self.sd_dense_layers(sd_x)
        sdmd_x = self.sdmd_dense_layers(sdmd_x_combined)
        md_x = self.md_dense_layers(md_x)

        #################
        # Output Layers #
        #################
        sd_x = self.output_layers[0](sd_x)
        sdmd_x = self.output_layers[1](sdmd_x)
        md_x = self.output_layers[2](md_x)

        return (sd_x, sdmd_x, md_x, attention_coefficients) if return_attention_coefficients else (sd_x, sdmd_x, md_x)

def hetero_global_pool(x_dict, batch_dict, pool_type='mean'):
    """
    Perform global pooling on heterogeneous graphs.

    Args:
        x_dict (dict): A dictionary containing node embeddings for each node type.
        batch_dict (dict): A dictionary containing batch information for each node type.
        pool_type (str): Type of pooling ('mean' or 'max').

    Returns:
        torch.Tensor: A single graph-level representation.
    """
    pooled_embeddings = []
    for node_type, x in x_dict.items():
        if pool_type == 'mean':
            pooled = global_mean_pool(x, batch_dict[node_type])
        elif pool_type == 'max':
            pooled = global_max_pool(x, batch_dict[node_type])
        else:
            raise ValueError(f"Unsupported pooling type: {pool_type}")
        
        pooled_embeddings.append(pooled)

    # Combine pooled embeddings from all node types
    graph_representation = torch.cat(pooled_embeddings, dim=1)
    return graph_representation


class DropSTNodes(BaseTransform):
    def __init__(self, max_nodes_to_drop: int):
        """
        Args:
            max_nodes_to_drop (int): Maximum number of ST nodes to drop per graph.
        """
        super(DropSTNodes, self).__init__()
        self.max_nodes_to_drop = max_nodes_to_drop

    def __call__(self, input_data: HeteroData) -> HeteroData:
        data = input_data.clone()  # Clone the input data to avoid modifying the original

        if "ST" not in data.node_types or not hasattr(data, "station_list") or not hasattr(data, "seed"):
            raise ValueError("The input data must have 'ST', 'station_list', and 'seed' attributes.")

        n_st_nodes = data["ST"].x.size(0)
        if n_st_nodes == 0:
            raise ValueError("The input data contains no ST nodes to drop.")

        # Step 1: Select ST nodes to drop
        seed_ids = set(data.seed)
        seed_indices = [i for i, station_id in enumerate(data.station_list) if station_id in seed_ids]

        droppable_indices = [i for i in range(n_st_nodes) if i not in seed_indices]
        if not droppable_indices:
            warnings.warn("No droppable ST nodes available (all nodes are seeds).")
            return data

        n_nodes_to_drop = random.randint(0, min(self.max_nodes_to_drop, len(droppable_indices)))
        nodes_to_drop = random.sample(droppable_indices, n_nodes_to_drop)

        keep_mask_st = torch.ones(n_st_nodes, dtype=torch.bool)
        keep_mask_st[nodes_to_drop] = False

        # Step 2: Create ST node mapping and update features
        relabel_map_st = torch.full((n_st_nodes,), -1, dtype=torch.long)
        relabel_map_st[keep_mask_st] = torch.arange(keep_mask_st.sum())

        data["ST"].x = data["ST"].x[keep_mask_st]
        data.station_list = [station_id for i, station_id in enumerate(data.station_list) if keep_mask_st[i]]
        data.pos = data.pos[keep_mask_st]

        # Update ST-to-ST edges
        for edge_type in list(data.edge_index_dict.keys()):
            src, _, dst = edge_type
            if src == "ST" and dst == "ST":
                edge_index = data[edge_type].edge_index
                src_relabel = relabel_map_st[edge_index[0]]
                dst_relabel = relabel_map_st[edge_index[1]]
                valid_mask = (src_relabel >= 0) & (dst_relabel >= 0)
                edge_index = edge_index[:, valid_mask]
                edge_index[0] = src_relabel[valid_mask]
                edge_index[1] = dst_relabel[valid_mask]
                data[edge_type].edge_index = edge_index

        # Step 3: Process other node types
        for node_type in data.node_types:
            if node_type == "ST":
                continue

            # Create mapping for the current node type
            n_nodes = data[node_type].x.size(0)
            keep_mask_node = torch.ones(n_nodes, dtype=torch.bool)

            # Identify nodes connected to dropped ST nodes
            for edge_type in list(data.edge_index_dict.keys()):
                src, _, dst = edge_type
                if src == "ST" and dst == node_type:
                    edge_index = data[edge_type].edge_index
                    connected_nodes = edge_index[1][relabel_map_st[edge_index[0]] == -1]
                    keep_mask_node[connected_nodes] = False

            relabel_map_node = torch.full((n_nodes,), -1, dtype=torch.long)
            relabel_map_node[keep_mask_node] = torch.arange(keep_mask_node.sum())

            # Update node features
            data[node_type].x = data[node_type].x[keep_mask_node]

            # Update edges for the current node type
            for edge_type in list(data.edge_index_dict.keys()):
                src, _, dst = edge_type
                if src == "ST" and dst == node_type:
                    edge_index = data[edge_type].edge_index
                    src_relabel = relabel_map_st[edge_index[0]]
                    dst_relabel = relabel_map_node[edge_index[1]]
                    valid_mask = (src_relabel >= 0) & (dst_relabel >= 0)
                    edge_index = edge_index[:, valid_mask]
                    edge_index[0] = src_relabel[valid_mask]
                    edge_index[1] = dst_relabel[valid_mask]
                    data[edge_type].edge_index = edge_index

                elif src == node_type and dst == "ST":
                    edge_index = data[edge_type].edge_index
                    src_relabel = relabel_map_node[edge_index[0]]
                    dst_relabel = relabel_map_st[edge_index[1]]
                    valid_mask = (src_relabel >= 0) & (dst_relabel >= 0)
                    edge_index = edge_index[:, valid_mask]
                    edge_index[0] = src_relabel[valid_mask]
                    edge_index[1] = dst_relabel[valid_mask]
                    data[edge_type].edge_index = edge_index

        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
    
        Args:
            batch (List[HeteroData]): A batch of heterogeneous graphs.
    
        Returns:
            HeteroData: Transformed batch.
        """
        # Transform each graph individually
        transformed_graphs = [self(graph) for graph in batch]
        
        # Use PyTorch Geometric's DataLoader collate function to combine the graphs
        batch = DataLoader.collate_fn(transformed_graphs)
        
        return batch


class DropNodesBySTSelection(BaseTransform):
    def __init__(self, max_st_nodes: int, target_node_type: str, drop_from_seed: bool = False):
        """
        Args:
            max_nodes_to_select (int): Maximum number of ST nodes to select per graph.
            target_node_type (str): The type of nodes connected to the selected ST nodes to drop.
            drop_from_seed (bool): Whether to drop nodes connected to seed ST nodes.
        """
        super(DropNodesBySTSelection, self).__init__()
        self.max_nodes_to_select = max_st_nodes
        self.target_node_type = target_node_type
        self.drop_from_seed = drop_from_seed

    def __call__(self, input_data: HeteroData) -> HeteroData:
        data = input_data.clone()  # Clone the input data to avoid modifying the original

        if "ST" not in data.node_types or self.target_node_type not in data.node_types:
            raise ValueError("The input data must have 'ST' and the specified target node type.")

        n_st_nodes = data["ST"].x.size(0)
        if n_st_nodes == 0:
            raise ValueError("The input data contains no ST nodes to select.")

        # Step 1: Select ST nodes to drop
        seed_ids = set(data.seed)
        seed_indices = [i for i, station_id in enumerate(data.station_list) if station_id in seed_ids]

        # Determine ST nodes eligible for selection
        if self.drop_from_seed:
            droppable_indices = list(range(n_st_nodes))  # Include all ST nodes, including seeds
        else:
            droppable_indices = [i for i in range(n_st_nodes) if i not in seed_indices]

        if not droppable_indices:
            warnings.warn("No droppable ST nodes available.")
            return data

        n_nodes_to_select = random.randint(0, min(self.max_nodes_to_select, len(droppable_indices)))
        nodes_to_select = random.sample(droppable_indices, n_nodes_to_select)

        keep_mask_st = torch.ones(n_st_nodes, dtype=torch.bool)
        keep_mask_st[nodes_to_select] = False

        # Step 2: Process the target node type
        n_target_nodes = data[self.target_node_type].x.size(0)
        keep_mask_target = torch.ones(n_target_nodes, dtype=torch.bool)
        relabel_map_target = torch.full((n_target_nodes,), -1, dtype=torch.long)

        for st_node in nodes_to_select:
            # Find target nodes connected to the current ST node
            for edge_type in list(data.edge_index_dict.keys()):
                src, _, dst = edge_type
                if src == "ST" and dst == self.target_node_type:
                    edge_index = data[edge_type].edge_index
                    connected_target_nodes = edge_index[1][edge_index[0] == st_node]
                    keep_mask_target[connected_target_nodes] = False

        # Update the relabel map for the target node type
        relabel_map_target[keep_mask_target] = torch.arange(keep_mask_target.sum())

        # Step 3: Update target node features
        data[self.target_node_type].x = data[self.target_node_type].x[keep_mask_target]

        # Step 4: Update target-related edges
        for edge_type in list(data.edge_index_dict.keys()):
            src, _, dst = edge_type
            if src == self.target_node_type or dst == self.target_node_type:
                edge_index = data[edge_type].edge_index
        
                # Relabel source and destination using the appropriate relabel maps
                src_relabel = (
                    relabel_map_target[edge_index[0]]
                    if src == self.target_node_type
                    else edge_index[0]
                )
                dst_relabel = (
                    relabel_map_target[edge_index[1]]
                    if dst == self.target_node_type
                    else edge_index[1]
                )
        
                # Filter out edges where relabeling failed (-1 indicates dropped nodes)
                valid_mask = (src_relabel >= 0) & (dst_relabel >= 0)
                edge_index = edge_index[:, valid_mask]
        
                # Relabel valid edges
                if src == self.target_node_type:
                    edge_index[0] = src_relabel[valid_mask]
                if dst == self.target_node_type:
                    edge_index[1] = dst_relabel[valid_mask]
        
                data[edge_type].edge_index = edge_index
        return data

    def collate(self, batch):
        """
        Collate function to apply the transformation to a batch of graphs.
    
        Args:
            batch (List[HeteroData]): A batch of heterogeneous graphs.
    
        Returns:
            HeteroData: Transformed batch.
        """
        # Transform each graph individually
        transformed_graphs = [self(graph) for graph in batch]
        
        # Use PyTorch Geometric's DataLoader collate function to combine the graphs
        batch = DataLoader.collate_fn(transformed_graphs)
        
        return batch


class CompositeTransform(BaseTransform):
    def __init__(self, transforms):
        """
        Args:
            transforms (List[BaseTransform]): A list of transformations to apply sequentially.
        """
        super(CompositeTransform, self).__init__()
        self.transforms = transforms

    def __call__(self, data):
        """
        Apply all transformations sequentially to a single graph.
        """
        for transform in self.transforms:
            data = transform(data)
        return data

    def collate(self, batch):
        """
        Apply all transformations sequentially to a batch of graphs.
        """
        for transform in self.transforms:
            batch = transform.collate(batch)
        return batch
