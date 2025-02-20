#!/bin/bash

# Setting up the environment
#module load Compilers/gcc/11.3.

eval "$(bash /sps/pauger/users/erodriguez/Software/offline_install/bin/this-auger-offline.sh)"

# Compile in case of changes
make

log_base="/sps/pauger/users/erodriguez/PhotonDiscrimination/ADST2JSON_log"

# Function to submit jobs
submit_jobs() {
    local particle="$1"
    local energy_bin="$2"
    local atm_versions=("$@")  # Array of atmospheric models starting from $3

    for atm in "${atm_versions[@]:2}"; do
        sbatch -t 23:59:59 -n 1 --mem 2G -A pauger -J "ADST2JSONCaller${particle}" \
            --output="${log_base}_${particle}_${energy_bin}_${atm}.txt" \
            ./caller.sh "$particle" "$energy_bin" "$atm"
    done
}

# Photon
#submit_jobs "Photon" "16.5_17.0" "01" "03" "08" "09"
#submit_jobs "Photon" "17.0_17.5" "01" "03" "08" "09"

# Proton
#submit_jobs "Proton" "16.5_17.0" "01" "03" "08" "09"
#submit_jobs "Proton" "17.0_17.5" "01" "03" "08" "09"

# Uncomment to enable Helium
#submit_jobs "Helium" "16.5_17.0" "01" "03" "08" "09"
#submit_jobs "Helium" "17.0_17.5" "01" "03" "08" "09"

# Uncomment to enable Iron
submit_jobs "Iron" "16.5_17.0" "01" "03" "08" "09"
submit_jobs "Iron" "17.0_17.5" "01" "03" "08" "09"
