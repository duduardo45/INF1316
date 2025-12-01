#!/bin/bash
set -e

gcc -Wall -Wextra -pedantic ./trab2/kernelSim.c ./trab2/state.c -o ./build/kernelSim
gcc -Wall -Wextra -pedantic ./trab2/interControllerSim.c -o ./build/interControllerSim
gcc -Wall -Wextra -pedantic ./trab2/A.c ./trab2/state.c -o ./build/A

exec ./build/kernelSim