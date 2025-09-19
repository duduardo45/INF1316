Vamos simular a troca de contexto usando uma área de memória compartilhada entre todos os processos, de forma a emular o estado do cpu, composto pelos
registradores.

Pipe para as interrupções que o interControlerSim faz no kernelSim.

# glossário
D1 -> device 1
D2 -> device 2
A -> programa user
IRQ -> interrupt request
PC -> Program Counter -> progresso do programa


# TODO
pensar como fazer o syscall do A pro kernel. pipe tambem?
