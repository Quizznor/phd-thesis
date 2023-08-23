#!/bin/bash

ssh -p22222 quizznor@localhost notify-send "'Take care =)' 'Enjoy your free(?) evening'"

# kill vscode remote server
if ! [[ -z "$(ps -ef | grep node)" ]]; then
    killall --user filip node
    ssh -p22222 quizznor@localhost "killall code-insiders"
fi

# kill system monitor
if ! [[ -z "$(ps -ef | grep htop)" ]]; then 
    killall --user filip htop
fi

