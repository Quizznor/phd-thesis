#/bin/bash

for n in $(seq 300); do
    ./do_calculations.py NuriaJr $n &
done