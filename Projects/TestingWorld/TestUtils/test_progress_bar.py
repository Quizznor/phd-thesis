import sys, os

sys.path.append('/cr/users/filip/bin')

from utils.binaries import *

from time import sleep

for n in tools.ProgressBar(range(0, 100), newline=False):
    sleep(0.1)