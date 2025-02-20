# Standard library imports
import os
import warnings

# Third party imports
import numpy as np
import pandas as pd
from typing import Any, List, Tuple, Union, Optional, Dict
     
######################
## Simple utilities ##
######################

def print_banner_text(middle_str: str) -> None:
    """
    Print the given text enclosed by "#" characters to create a banner.

    Parameters:
    - middle_str (str): The text to be placed in the middle of the banner.
    """
    middle_str = "## " + middle_str + " ##"
    banner_length = len(middle_str)

    print("")
    print("#" * banner_length)
    print(middle_str)
    print("#" * banner_length)
    print("")
    
def get_scientific_notation_label(number: float, decimal_places: int = 2) -> str:
    """
    Convert a number to scientific notation string with the specified decimal places.

    Args:
        number (float): The number to convert.
        decimal_places (int, optional): The number of decimal places in the result. Defaults to 2.

    Returns:
        str: The number in scientific notation as a string.
    """
    if number == 0.0:
        return f"0.00e+00"
    
    exponent = 0
    while abs(number) >= 10.0:
        number /= 10.0
        exponent += 1
    while abs(number) < 1.0 and number != 0.0:
        number *= 10.0
        exponent -= 1
    formatted_number = f"{number:.{decimal_places}f}e{exponent:+03d}"
    return formatted_number

def flatten_list(nested_list: List[Union[int, List]]) -> List[int]:
    """
    Flatten a nested list, returning a flat list of integers.

    Args:
        nested_list (List[Union[int, List]]): The nested list to be flattened.

    Returns:
        List[int]: A flat list containing only integers.
    """
    flat_result = []

    def recursive_flatten(sublist):
        for item in sublist:
            if isinstance(item, int):
                flat_result.append(item)
            elif isinstance(item, list):
                recursive_flatten(item)

    recursive_flatten(nested_list)
    return flat_result
    
###########################
## pd.DataFrame Related  ##
###########################

def filter_dataframe(
    df: pd.DataFrame, 
    feature_filters: Optional[Dict[str, Dict[str, Union[str, float]]]] = None) -> pd.DataFrame:
    """
    Filter a DataFrame based on boolean parameters and cut values for different features, including range and value-based filters.

    Parameters:
    - df (pd.DataFrame): The input DataFrame to be filtered.
    - feature_filters (dict, optional): A dictionary where keys are feature names and values are dictionaries
      containing the following keys:
      - 'filter_type' (str): 'range' for range filters (min-max), 'value' for value-based filters.
      - 'min_cut' (float, optional): Minimum value to include in the filtered DataFrame (inclusive) for range filters.
      - 'max_cut' (float, optional): Maximum value to include in the filtered DataFrame (inclusive) for range filters.
      - 'value_to_keep' (Union[str, float], optional): Value to keep for value-based filters.
      - 'value_to_reject' (Union[str, float], optional): Value to reject for value-based filters.

    Returns:
    - pd.DataFrame: Filtered DataFrame.

    Example usage:
    ```python
    data = {
        'Feature1': [1, 2, 3, 4, 5],
        'Feature2': ['A', 'B', 'A', 'C', 'B'],
    }

    df = pd.DataFrame(data)

    feature_filters = {
        'Feature1': {'filter_type': 'range', 'min_cut': 2, 'max_cut': 4},
        'Feature2': {'filter_type': 'value', 'value_to_keep': 'A'},
    }

    filtered_df = filter_dataframe(df, feature_filters)
    print(filtered_df)
    ```
    """
    if feature_filters is None:
        return df

    if not feature_filters:
        warnings.warn("The 'feature_filters' dictionary is empty. No filtering will be applied.")

    filtered_df = df.copy()

    for feature, filter_info in feature_filters.items():
        filter_type = filter_info.get('filter_type')

        if filter_type is None:
            raise ValueError(f"Filter type not provided for feature '{feature}'.")

        if filter_type == 'range':
            min_cut = filter_info.get('min_cut', float('-inf'))
            max_cut = filter_info.get('max_cut', float('inf'))

            if min_cut > max_cut:
                raise ValueError(f"Invalid range filter for feature '{feature}': min_cut should be less than or equal to max_cut.")
            
            if 'min_cut' not in filter_info and 'max_cut' not in filter_info:
                warnings.warn(f"Both min_cut and max_cut are not provided for range filter on feature '{feature}'. This will include all values for the feature.")

            filtered_df = filtered_df[
                (filtered_df[feature] >= min_cut) &
                (filtered_df[feature] <= max_cut)
            ]
        elif filter_type == 'value':
            value_to_keep = filter_info.get('value_to_keep')
            value_to_reject = filter_info.get('value_to_reject')

            if value_to_keep is None and value_to_reject is None:
                raise ValueError(f"Invalid value filter for feature '{feature}': Specify 'value_to_keep' or 'value_to_reject'.")

            if value_to_keep is not None and value_to_reject is not None:
                raise ValueError(f"Invalid value filter for feature '{feature}': Specify either 'value_to_keep' or 'value_to_reject', not both.")

            if 'value_to_keep' not in filter_info and 'value_to_reject' not in filter_info:
                warnings.warn(f"Neither value_to_keep nor value_to_reject is provided for value filter on feature '{feature}'. No values will be filtered based on this feature.")

            if value_to_keep is not None:
                filtered_df = filtered_df[filtered_df[feature] == value_to_keep]
            elif value_to_reject is not None:
                filtered_df = filtered_df[filtered_df[feature] != value_to_reject]

    return filtered_df

def add_zenith_related_columns(
    df: pd.DataFrame,
    zenith_column: str,
    zenith_in_degrees: bool = True,
    reference_zenith: Union[float, int] = 30.0
) -> pd.DataFrame:
    """
    Calculate and add new columns related to zenith angle to the DataFrame.

    Parameters:
    - df (pd.DataFrame): The input DataFrame.
    - zenith_column (str): The name of the column containing zenith angle values.
    - zenith_in_degrees (bool, optional): Set to True if the zenith values are in degrees (default),
      set to False if they are in radians.
    - reference_zenith (float or int, optional): The reference zenith angle for 'x' calculation. It should be
      in the same units as the input zenith angle.

    Returns:
    - pd.DataFrame: The DataFrame with the new columns added.

    Description:
    This function calculates and adds the following columns to the DataFrame:
    - 'sin2zenith': The square of the sine of the zenith angle.
    - 'x': The difference between 'sin2zenith' and the square of the sine of the reference zenith angle.
      'x' represents a measure related to the deviation of the zenith angle from the reference zenith angle.
    - 'secant_zenith': The secant of the zenith angle.

    The 'zenith_in_degrees' parameter allows you to specify whether the zenith values in the 'zenith_column'
    are in degrees (default) or radians.

    The 'reference_zenith' parameter allows you to set the reference zenith angle for 'x' calculation, and it
    should be in the same units as the input zenith angle.
    """

    zenith_rad = np.deg2rad(df[zenith_column]) if zenith_in_degrees else df[zenith_column]

    sin2zenith = np.sin(zenith_rad) ** 2
    x = sin2zenith - np.sin(np.deg2rad(reference_zenith)) ** 2
    secant_zenith = 1 / np.cos(zenith_rad)

    df["sin2zenith"] = sin2zenith
    df["x"] = x
    df["secant_zenith"] = secant_zenith

    return df

def create_bins(df: pd.DataFrame, lower_val: float = 10**16.0, upper_val: float = 10**17.0,
                num: int = 11, unbinned_col: str = "energyMC", bin_column_name: str = "e_bin",
                bin_width: str = "equal") -> Tuple[pd.DataFrame, List[float], List[float], List[str]]:
    """
    Create bins for a DataFrame column.

    Args:
        df (pd.DataFrame): The input DataFrame.
        lower_val (float): The lower bound of bin values.
        upper_val (float): The upper bound of bin values.
        num (int): The number of bins.
        unbinned_col (str): The column to be binned.
        bin_column_name (str): The name of the new column for bin labels.
        bin_width (str): The binning method ("equal", "equal_logarithmic", or "quantile").

    Returns:
        Tuple[pd.DataFrame, List[float], List[float], List[str]]: A tuple containing:
            - The modified DataFrame with bin labels.
            - The bin centers.
            - The bin edges.
            - The bin labels.
    """
    if bin_width == "equal":
        bin_edges = np.linspace(lower_val, upper_val, num=num+1)
    elif bin_width == "equal_logarithmic":
        bin_edges = np.logspace(np.log10(lower_val), np.log10(upper_val), num=num+1)
    elif bin_width == "quantile":
        df[bin_column_name], qbin_edges = pd.qcut(x=df[unbinned_col], q=num+1, retbins=True, labels=False)
        qbin_centers = [(qbin_edges[i] + qbin_edges[i+1]) / 2 for i in range(len(qbin_edges)-1)]
        qbin_labels = [f"{int(np.floor(qbin_edges[i]))}-{int(np.floor(qbin_edges[i+1]))}" for i in range(len(qbin_edges)-1)]
        return df, qbin_centers, qbin_edges, qbin_labels
    else:
        raise ValueError('The parameter bin_width should be either "equal", "equal_logarithmic", or "quantile".')
    
    bin_centers = [(bin_edges[i] + bin_edges[i+1]) / 2 for i in range(len(bin_edges)-1)]
    labels = [f"{get_scientific_notation_label(bin_edges[i], 2)} - {get_scientific_notation_label(bin_edges[i+1], 2)}" for i in range(len(bin_edges)-1)]    
    df[bin_column_name] = pd.cut(x=df[unbinned_col], bins=bin_edges, labels=labels, include_lowest=True)
    
    return df, bin_centers, bin_edges, labels

