#!/bin/bash
set -e

gcc -Wall -Wextra -pedantic ./trab1/kernelSim.c ./trab1/state.c -o ./build/kernelSim
gcc -Wall -Wextra -pedantic ./trab1/interControllerSim.c -o ./build/interControllerSim
gcc -Wall -Wextra -pedantic ./trab1/A.c ./trab1/state.c -o ./build/A

exec ./build/kernelSim