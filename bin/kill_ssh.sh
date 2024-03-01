#!/bin/bash

# ssh -p22222 quizznor@localhost notify-send "'Take care =)' 'Enjoy your free(?) evening'"

# kill vscode remote server
if ! [[ -z "$(ps -ef | grep '^filip.*node [^\$]*$')" ]]; then
    ps uxa --user filip | grep .vscode-server | awk '{print $2}' | xargs kill -9
    ps uxa --user filip | grep ipykernel_launcher | awk '{print $2}' | xargs kill -9
    # ssh -p22222 quizznor@localhost "killall code-insiders"
fi

# # kill system monitor
# if ! [[ -z "$(ps -ef | grep '^filip.*htop$')" ]]; then 
#     killall --user filip htop
# fi

