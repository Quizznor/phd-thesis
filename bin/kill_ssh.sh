#!/bin/bash

ssh -p22222 quizznor@localhost notify-send "'Take care =)' 'Enjoy your free(?) evening'"

# kill vscode remote server
if ! [[ -z "$(ps -ef | grep '^filip.*node [^\$]*$')" ]]; then
    ssh -p22222 quizznor@localhost "killall code-insiders"
    killall --user filip node
fi

# kill system monitor
if ! [[ -z "$(ps -ef | grep '^filip.*htop --user filip$')" ]]; then 
    killall --user filip htop
fi

