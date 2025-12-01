# Direções para o trabalho N°1
Vamos simular a troca de contexto usando uma área de memória compartilhada 
entre todos os processos, de forma a emular o estado do cpu, composto pelos registradores.

Pipe para as interrupções que o interControlerSim faz no kernelSim.

Pensar em implementar o PC como um switch case que não inclua breaks para que o processo consiga recuperar qualquer ponto do loop.
**ESTÁ ESCRITO QUE PC É PARA SER UM CONTADOR DE INTERAÇÕES.**

Considerar se vale a pena fazer o controle de só enviar o syscall pro DX quando esse não estiver busy (criar um work queue para os DX)
Poderia fazer com que cada ação demorasse diferente (R,W,X)

Deve-se iniciar os estados dos A2+ no kernelSim?

Implementar a subida e descida de prioridade dos processos dependendo
se usou ou não toda a sua fatia de tempo

---

### glossário
- D1 -> device 1
- D2 -> device 2
- AX -> programa user
- IRQ -> interrupt request
- PC -> Program Counter -> progresso do programa


# Direções para a parte 2

Primeiro implementando uma primeira iteração usando só operação de RD e sem A0.

Flow dessa versão básica:
- processo A1 faz uma syscall, passando syscall_args pro kernelSim por meio da fifo.
- kernelSim recebe a syscall e reage a ela, parando A1 e colocando ele na fila de espera do IRQ1 (porque é operação de arquivo e não diretorio)
- kernelSim usa o protocolo UDP para mandar request ao SFSS
- (não achei instruções no pdf sobre a implementação do SFSS, só sei que vai ser um servidor UDP e que vai responder um UDP pro kernelSim)
- kernelSim recebe a resposta UDP e coloca no File-Request-Queue, uma nova fila de respostas de arquivo do sfss
- em um momento aleatório, interControllerSim solta um IRQ1
- kernelSim para o processo atual, pega a resposta mais antiga presente em File-Request-Queue. Salva ela no shared memory daquele processo (será que precisa criar um novo shmem pra cada processo, como parece pedir o pdf? Ou podemos usar o state do processo que já temos para conter a resposta?).
- Acorda o processo. Ele vai ver a resposta ao olhar no shmem dele