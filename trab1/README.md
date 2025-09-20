# Direções para o trabalho N°1
Vamos simular a troca de contexto usando uma área de memória compartilhada 
entre todos os processos, de forma a emular o estado do cpu, composto pelos registradores.

Pipe para as interrupções que o interControlerSim faz no kernelSim.

Pensar em implementar o PC como um switch case que não inclua breaks para que o processo consiga recuperar qualquer ponto do loop.
**ESTÁ ESCRITO QUE PC É PARA SER UM CONTADOR DE INTERAÇÕES.**

Considerar se vale a pena fazer o controle de só enviar o syscall pro DX quando esse não estiver busy (criar um work queue para os DX)
Poderia fazer com que cada ação demorasse diferente (R,W,X)

Deve-se iniciar os estados dos A2+ no kernelSim?

---

### glossário
- D1 -> device 1
- D2 -> device 2
- AX -> programa user
- IRQ -> interrupt request
- PC -> Program Counter -> progresso do programa

---

### TODO
- Para o A.c:
  - pensar como fazer o syscall do AX pro kernel. pipe tambem?
  - pensar em como fazer os AX terem acesso à área compartilhada do estado do núcleo.
  - fazer o A.c receber command line args para fazer os programas terem diferentes chances de fazer uma syscall e de chamar D1 ou D2.


- Para o kernelSim.c:
  - completar pseudo código
  - escrever a implementação


- Para o interControllerSim.c:
  - fazer tudo?


- Para o state.h:
  - refatorar State para só considerar o que pode mudar durante a execução do AX? Talvez não seja um problema.
  - trocar o nome para ficar de acordo com o nome correto para "contexto de execução da CPU" porque acredito que o Endler deu um nome fechado para isso.