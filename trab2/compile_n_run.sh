#!/bin/bash
set -e

mkdir -p ./build

# Compilação dos componentes
gcc -Wall -Wextra -pedantic ./trab2/kernelSim.c ./trab2/state.c -o ./build/kernelSim
gcc -Wall -Wextra -pedantic ./trab2/interControllerSim.c -o ./build/interControllerSim
gcc -Wall -Wextra -pedantic ./trab2/A.c ./trab2/state.c -o ./build/A

# SFSS agora precisa incluir diretórios se state.h estiver lá, mas como está no mesmo dir:
gcc -Wall -Wextra -pedantic ./trab2/sfss.c -o ./build/sfss

# Ignora Ctrl+Z (SIGTSTP) para evitar suspensões indesejadas
trap '' SIGTSTP

# Roda SFSS em background
./build/sfss &
SFSS_PID=$!

sleep 1 # Dá um tempo para o servidor subir

cleanup() {
    echo "Encerrando SFSS (PID: $SFSS_PID)..."
    kill $SFSS_PID 2>/dev/null || true
}

# Registra a função cleanup para rodar SEMPRE que o script sair (EXIT),
# seja por fim natural, erro ou sinal (Ctrl+C)
trap cleanup EXIT

# Roda KernelSim (que vai subir InterController e Apps)
./build/kernelSim