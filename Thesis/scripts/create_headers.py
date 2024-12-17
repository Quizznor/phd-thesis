#!/usr/bin/env python

import sys, os

hostname = os.uname()[1]

if "crc" in hostname:
    scriptdir = "/cr/users/filip/bin/"
    basedir = "/cr/data01/filip/.history/"
    savedir = "/cr/users/filip/Thesis/header/"
elif "beep-boop" == hostname:
    scriptdir = "/home/quizznor/projects/phd-thesis/bin/"
    basedir = "/home/quizznor/projects/phd-thesis/Data/activity/"
    savedir = "/home/quizznor/projects/phd-thesis/Thesis/header"

sys.path.append(scriptdir)

from utils.binaries import *
from utils.plotting import *

import pickle
import datetime

phd_start, phd_end = datetime.datetime(2023, 8, 1), datetime.datetime(2026, 7, 31)

cmds = [
    "git",  # Introduction
    "lyon",  # Pierre Auger Observatory
    "git",  # Bibliography
    "ls",  # Hallo
]

titles = [
    "Introduction",
    "The part where everything begins",
    "The part where everything begins",
    "Acknowledgements",
]

quotes = [
    [
        "In the beginning the universe was created. This has made a lot of people angry, and has been widely regarded as a bad move",
        "Douglas Adams, The Restaurant at the End of the Universe",
    ],
    [
        "I am happy you are here with me. Here at the end of all things, Sam",
        "Frodo Baggins, Return of the King",
    ],
    [
        "I am happy you are here with me. Here at the end of all things, Sam",
        "Frodo Baggins, Return of the King",
    ],
    [
        "If I have seen further, it is by standing on the shoulders of giants",
        "Sir Isaac Newton",
    ],
]


def make_tooltip(i):
    this_title = titles[i]
    quote, author = quotes[i]

    return f"{i}. {this_title}\n\n{quote}\n{f'- {author}'.rjust(len(quote))}"


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
    if hour == "README.md":
        continue

    timestamp = datetime.datetime(
        *[int(i) for i in hour[:-7].split("_")]
    ).timestamp() + np.random.randint(3600)

    with open(basedir + hour, "rb") as f:
        history = pickle.load(f)

    timestamps = get_timestamps(history, timestamp)
    all_of_history = add_to_dict(all_of_history, timestamps)
    timestamps = get_timestamps(history, timestamp)
    all_of_history = add_to_dict(all_of_history, timestamps)

"""
Maybe create some function to massage data here? I.e. put all condor_* into one category?
"""
keys_to_delete = []
for key, value in all_of_history.items():
    if len(value) < 10:
        keys_to_delete.append(key)

for key in keys_to_delete:
    del all_of_history[key]


# dict modification ##############################################################

# add lyon keyword
all_of_history["lyon"] = []
for key in ["ssh", "./copy_data_to_iap.sh", "./copy_to_iap.sh"]:
    all_of_history["lyon"] += all_of_history[key]

##################################################################################

plt.rcParams["figure.figsize"] = [11, 2.5]

for i, cmd in enumerate(cmds, 0):

    tooltip = make_tooltip(i)

    plt.figure()
    data = all_of_history[cmd]
    for year in [2024, 2025, 2026]:
        plt.axvline(datetime.datetime(year, 1, 1).timestamp(), 0, 0.066, c="k", ls="--")

    line = tool.kd1d_estimate(data, bandwidth=3e5)
    X = np.linspace(min(data), max(data), 10000)

    plt.plot(X, line(X), lw=3, c="#009999")
    plt.xlim(phd_start.timestamp(), phd_end.timestamp())
    plt.xticks([])
    plt.yticks([])
    plt.axis("off")
    so.rugplot(data, c="k", alpha=0.1)
    plt.legend(loc="lower right", title=cmd, title_fontsize=10)
    plt.savefig(f"{savedir}/{i}.png")

    format = f"{tooltip}\n\ngit\n{{"
    n_chars = len(str(data))
    width = np.sqrt(5 / 3 * n_chars)

    matrix = ""
    row = ""

    for number in data:
        if len(row) // width:
            matrix += "\n\t" + row
            row = ""

        row += str(hex(number))[2:].upper() + " "

    format += matrix + "\n}"

    with open(f"{savedir}/{i}.txt", "w") as qr_code:
        qr_code.write(format)
