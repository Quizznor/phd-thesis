#!/bin/python3

import os
import sys
import pickle

"""
This script is meant to back up commands executed over the command line for stats purposes
In order for this script to be run before every command, amend your $HOME/.bashrc like so:

>>>
function keep_stats() {
	<path/to/this/script> $BASH_COMMAND
}

trap keep_stats DEBUG
<<<

It is expected that readling/writing a large pickle file will slow down execution of EVERY
command noticeably. For this reason, it is recommended to regularly back up your history.pickle
regularly, e.g. with crontab, like so:

>>>
# (backs up history.pickle every hour, datetime is in server time! (=UTC))
0 * * * * bash <path/to/below/executable>

if [ -f "$HOME/bin/history.pickle" ]; then
    mv $HOME/bin/history.pickle /cr/data01/$USER/.history/$(date '+%Y_%m_%_d_%H').pickle
f
<<<

"""


def print_history(history):
    max_key_len = max([len(key) for key in history.keys()])
    for key, item in history.items():
        print(f"{key: <{max_key_len}}: {item * '|': >{116-max_key_len}}")


pickle_location = f"/cr/users/{os.getlogin()}/bin/history.pickle"

# # main script for stats keeping
if __name__ == "__main__":

    try:
        with open(pickle_location, "rb") as f:
            history = pickle.load(f)
    except FileNotFoundError:
        history = {}

    try:
        # make it possible to just print and exit
        if sys.argv[1] == "list":
            print_history(history)
            sys.exit()
        # don't save stuff written from vscode
        elif sys.argv[1].startswith("__vsc") or "node" in sys.argv[1]:
            sys.exit()
        # don't save stats on login / logout
        elif sys.argv[1] == "/cr/users/filip/bin/exit.sh" or sys.argv[1].startswith(
            "PATH"
        ):
            sys.exit()
        # whatever calls these things
        elif sys.argv[1] in ["[", "[[", "'", '"', "builtin", "unset", "trap"]:
            sys.exit()
        # keep stats on everything else
        else:
            try:
                history[sys.argv[1]] += 1
            except KeyError:
                history[sys.argv[1]] = 1

            with open(pickle_location, "wb") as f:
                pickle.dump(history, f, pickle.HIGHEST_PROTOCOL)
    except IndexError:
        pass
