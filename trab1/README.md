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