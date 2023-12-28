This folder contains (almost) the entire history of my bash shell during my PhD at the IAP from August 2023 to its completion. The history - if 
there has been a new command - is backed up hourly by a crontab. File names follow the format `YYYY_MM_DD_HH`, where `HH` refers to the hour at 
which the history was saved. This is to say that `2023_08_18_14.pickle` contains all bash commands issued from 1pm to 2pm on the 18th august 2023.

The files themselves are packed in binary format via python. Read them like so:

    import pickle
    
    with open("path/to/history/file", "rb") as f:
        history = pickle.load(f)
