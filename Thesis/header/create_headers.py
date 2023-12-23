#!/usr/bin/env python

import sys, os

hostname = os.uname()[1]

if "crc" in hostname:
    scriptdir = "/cr/users/filip/bin/"
    basedir = "/cr/data01/filip/.history/"
elif "beep-boop" == hostname:
    scriptdir = "/home/quizznor/projects/phd-thesis/bin/"
    basedir = "/home/quizznor/projects/phd-thesis/Data/"

sys.path.append(scriptdir)

from utils.binaries import *
from utils.plotting import *

import pickle
import datetime

phd_start, phd_end = datetime.datetime(2023, 8, 1), datetime.datetime(2026, 7, 31)

def add_to_dict(original, new):

    for key, value in new.items():
        try:
            original[key] += value
        except KeyError:
            original[key] = value

    return original

def get_timestamps(original, timestamp):

    new = {}

    for key, value in original.items():
        new[key] = [int(timestamp + np.random.randint(3600)) for _ in range(value)]
    
    return new

all_of_history = {}

for hour in os.listdir(basedir):
    if hour == "README.md": continue
    
    timestamp = datetime.datetime(*[int(i) for i in hour[:-7].split("_")]).timestamp() + np.random.randint(3600)

    with open(basedir + hour, "rb") as f: 
        history = pickle.load(f)

    timestamps = get_timestamps(history, timestamp)
    all_of_history = add_to_dict(all_of_history, timestamps)
    timestamps = get_timestamps(history, timestamp)
    all_of_history = add_to_dict(all_of_history, timestamps)

"""
Maybe create some function to massage data here? I.e. put all condor_* into one category?
"""

cmds = [
    "git",          # Introduction
]

tooltips = [
    "1. The part where everything begins\n\nCollaboration through meetings.\t- Paul Filip",
]

for i, (tooltip, cmd) in enumerate(zip(tooltips, cmds), 1):

    plt.figure()
    data = all_of_history[cmd]
    for year in [2024, 2025, 2026]:
        plt.axvline(datetime.datetime(year, 1, 1).timestamp(), 0, .066, c="k", ls="--")

    line = tool.kd1d_estimate(data, bandwidth=3e5)
    X = np.linspace(min(data), max(data), 10000)


    plt.rcParams["figure.figsize"] = [11, 2.5]
    plt.plot(X, line(X), lw=3, c="#009999")
    plt.xlim(phd_start.timestamp(), phd_end.timestamp())
    plt.xticks([])
    plt.yticks([])
    plt.axis("off")
    so.rugplot(data, c="k", alpha=0.1)
    plt.legend(loc="upper left", title=cmd, title_fontsize=10)
    plt.savefig(f"{i}.png")

    format = f"{tooltip}\n\ngit\n{{"
    n_chars = 0.3 * len(str(data))
    n_sqrt = int(np.floor(np.sqrt(n_chars)))

    matrix = ""
    row = ""

    for number in np.random.choice(data, 30):
        if len(row) // n_sqrt:
            matrix += "\n\t" + row
            row = ""
        
        row += str(hex(number))[2:].upper() + " "

    format += matrix + "\n}"

    with open(f"{i}.txt", "w") as qr_code:
        qr_code.write(format)