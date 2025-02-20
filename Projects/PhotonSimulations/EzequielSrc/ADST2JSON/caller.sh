#!/bin/bash

# Ensure the caller script receives three arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <primary> <energy_bin> <atm>"
    exit 1
fi

# Assign script arguments to variables
primary="$1"
energy_bin="$2"
atm="$3"

# Define file paths
indexFile_hadron="/sps/pauger/users/erodriguez/PhotonDiscrimination/index_hadron_rec_${primary}_${energy_bin}_${atm}.csv"
indexFile_photon="/sps/pauger/users/erodriguez/PhotonDiscrimination/index_photon_rec_${primary}_${energy_bin}_${atm}.csv"

# Define directories
JSON_ROOT_DIR="/sps/pauger/users/erodriguez/PhotonDiscrimination/JSONfiles"
JSON_PRIM_DIR="${JSON_ROOT_DIR}/${primary}"

# Function to create a directory if it doesn't exist
create_dir_if_not_exists() {
    local dir="$1"
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir"
        echo "Directory created: $dir"
    else
        echo "Directory already exists: $dir"
    fi
}

# Create necessary directories
create_dir_if_not_exists "$JSON_ROOT_DIR"
create_dir_if_not_exists "$JSON_PRIM_DIR"

# Load necessary modules and environment
#module load Compilers/gcc/11.3.0

eval "$(bash /sps/pauger/users/erodriguez/Software/offline_install/bin/this-auger-offline.sh)"

# Function to remove and create index files
setup_index_file() {
    local file="$1"
    local header="$2"
    if [ -e "$file" ]; then
        rm "$file"
        echo "Removed existing file: $file"
    fi
    touch "$file"
    echo "$header" >> "$file"
    echo "Created new file with header: $file"
}

# Setup index files with headers
setup_index_file "$indexFile_hadron" "filename,atm_model,shower_id,use_id,energyMC,zenithMC,showerSize,showerSizeError,trigger,isT5,is6T5,Xmax,hottestid,nearestid,nCandidates,bLDF,isSaturated,muonNumber,electromagneticEnergy"
setup_index_file "$indexFile_photon" "filename,atm_model,shower_id,use_id,photon_energy,s_250,equivalent_energy,M1"

# Function to run ADST2JSON commands
run_adst2json() {
    local index_file="$1"
    local json_dir="$2"
    local pattern="$3"
    echo "Running ADST2JSON"
    # Perform the expansion inside the function
    ./ADST2JSON "$index_file" "$json_dir" $pattern
    echo ""
}

# Function to run ADSTappend2JSON commands
run_adstappend2json() {
    local index_file="$1"
    local json_dir="$2"
    local pattern="$3"
    echo "Running ADSTappend2JSON"
    # Perform the expansion inside the function
    ./ADSTappend2JSON "$index_file" "$json_dir" $pattern
}

# Run ADST2JSON and ADSTappend2JSON
run_adst2json "$indexFile_hadron" "$JSON_PRIM_DIR" "/sps/pauger/users/erodriguez/PhotonDiscrimination/${primary}/${energy_bin}/${atm}*/ADST_hadron.root"
run_adstappend2json "$indexFile_photon" "$JSON_PRIM_DIR" "/sps/pauger/users/erodriguez/PhotonDiscrimination/${primary}/${energy_bin}/${atm}*/ADST_photon.root"
