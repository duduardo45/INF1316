# Relatório do T2

Este relatório é derivado do relatório do T1, com alterações para descrever as mudanças no projeto.
Nossa dupla é:
- Luiz Felipe Neves Batista - 2311024
- Eduardo Eugênio de Souza - 2310822

# Introdução

O trabalho foi dividido nos seguintes componentes principais:
- KernelSim.c, o núcleo simulado
- interControllerSim.c, o gerador de interrupções simulado
- A.c, que é instanciado várias vezes para representar os processos de aplicação
- sfss.c, que é a implementação do Simple File System Server
- state.c, um arquivo com algumas funcionalidades auxiliares

O projeto pode ser rodado usando o arquivo
compile_n_run.sh, que possui comandos para compilar
os arquivos do projeto e rodar KernelSim como ponto de entrada. Ele também inicia o SFSS,
que roda em paralelo ao KernelSim, e utiliza o comando "trap" para ignorar suspensões do script em si e 
limpar o processo do SFSS quando o script é terminado por ctrl-c.

Utilizamos, tanto para as interrupções quanto para as chamadas de sistema, o mecanismo de comunicação
entre processos chamado FIFO. KernelSim lê ambos os FIFOs, enquanto interController escreve no FIFO
de interrupções (irq_fifo) e A.c escreve no FIFO de system calls (syscall_fifo).

Para modelar o estado atual de uma CPU de verdade, usamos memória compartilhada (shmem)
de forma que os processos precisem ser coordenados para evitar instruções incorretas ou perda de dados.
KernelSim deve realizar as trocas de contexto corretamente, de forma a somente permitir que um dos processos
de aplicação rode em um dado momento, carregando a CPU simulada com o estado desse processo.

KernelSim também deve tratar corretamente as interrupções (IRQ0, IRQ1 e IRQ2), terminando o processo atual e iniciando
um processo pronto para executar (caso seja IRQ0) ou que estava esperando uma chamada de sistema terminar (caso seja IRQ1 ou IRQ2).

Além disso, como adição no T2, KernelSim se comunica com o SFSS por meio de pacotes UDP, realizando pedidos 
a ele depois que os processos de aplicação fazem syscalls. Após a resposta, ele deve retornar o resultado
das syscalls para os processos de aplicação.


## KernelSim

A função main fica focada, primeiramente, na inicialização das estruturas de dados, da camada de rede e dos outros processos,
e, posteriormente, na leitura contínua dos dois FIFOs supracitados e do socket UDP.

KernelSim inicializa interControllerSim por meio de um fork e exec, fazendo o mesmo para criar
os processos de aplicação com o arquivo A.c. Também é no KernelSim que é registrado o tratador
de sinal para SIGSTP do projeto como um todo, permitindo que façamos uma pausa manual
e inspecionemos várias informações úteis. Também temos um tratador de sinal para Ctrl-C (SIGINT) de forma
a limpar memória compartilhada, fecha o socket e mata os outros processos no momento em que o KernelSim é finalizado.

Uma vez decidido que KernelSim deveria ler tanto a FIFO das interrupções quanto a FIFO das chamadas de sistema (e também
o socket do UDP), ficou evidente que um simples read() não seria ideal, porque bloquearia até que houvesse algo naquele File
descriptor, impedindo o recebimento de informações de outro. Seria possível usar a opção não bloqueante do read(), mas preferimos usar
a função pselect (variante de select), a qual permite "escutar" mudanças em mais de um descritor de arquivo sem gastar tempo
de CPU desnecessariamente.

Quando KernelSim detecta uma mudança em algum dos FIFOs, ele para o processo atual usando SIGSTOP (se a CPU não estava ociosa) e 
chama a função correspondente: handle_syscall ou handle_irq.

Essas funções, internamente, lidam com as filas de pronto e de espera por IRQ1 e IRQ2, de forma a colocar o processo
atual em alguma lista, se necessário, e acordar o processo correto após uma interrupção ou syscall. Além disso, ambas as funções handle_syscall e handle_irq chamam a função switch_context, que realiza a troca de contexto ao mudar o ponteiro referente à CPU e salvar o
contexto do processo que foi parado. Ela também lida tanto com os casos em que a CPU estava ociosa e agora deve executar um
processo quanto com os casos em que a CPU estava executando um processo e agora deve ficar ociosa.

Então, ele começa a executar o processo apontado pela CPU (desde que não seja caso de ficar ociosa).

A função handle_syscall insere o processo que fez a syscall na fila correta: a fila de IRQ1 inclui os processos que fizeram chamadas de
sistema para operações de arquivo, enquanto IRQ2 inclui os referentes a operações de diretório. Ademais, ela
chama send_request_to_sfss, que envia uma struct SfssRequest (construída a partir da syscall_args) através do socket do UDP,
como um pedido ao SFSS.

A função handle_udp_response é chamada quando são detectados novos dados no socket do UDP. Ela é responsável por adicionar a resposta que veio
do SFSS na fila de respostas correta: File-Request-Queue (nossa file_response_queue) ou Dir-Request-Queue (nossa dir_response_queue).

A função handle_irq precisa acordar o processo da fila correta. Mas antes disso, ela se certifica de que a resposta mais recente recebida do
SFSS é de fato para o primeiro processo da fila de IRQ correspondente (trata-se de uma verificação adicional para garantir que o processo sempre
recebe a resposta do pedido que fez). Caso não, ela continua procurando na fila de respostas até encontrar a resposta destinada a aquele processo.
Se essa resposta não for encontrada por algum motivo desconhecido (talvez uma demora excessiva na execução do SFSS), esse processo vai para
o fim da fila de IRQ em que estava, para que em algum momento futuro tente receber sua resposta novamente.
Quando a resposta é encontrada, o estado do processo é atualizado para que inclua a struct syscall_response que estava na fila de respostas. Assim,
o processo poderá acessar a resposta quando for acordado.

## SFSS

O Simple File System Server (SFSS) é o responsável por realizar as operações atômicas sobre as pastas e arquivos
dos processos, sem guardar estado (é stateless) Ele recebe os comandos em um formato de datagrama UDP conforme pedido pelo trabalho. Sobre isto, decidimos fazer dois formatos diferentes entre request e response, criando os tipos SfssRequest e SfssResponse para encapsular o
datagrama.

Internamente ao código, decidimos modularizar as funções de handling para cada uma das operações suportadas, e que são:
    - RD (read file)
    - WR (write file)
    - DC (dir create)
    - DR (delete)
    - DL (dir list)

Além disso vale ressaltar a decisão feita de ao invés de representar o "owner" mencionado no enunciado pelo seu PID ou nome "Ax",
optamos por usar a posição do processo no vetor de todos os processos, para simplificar a contagem.
O SFSS também reconstrói o path do sistema operacional correto de acordo com o "path lógico" dado pelo processo, para conseguir
de fato operar em cima dos arquivos e diretórios reais do computador.

Se alguma das operações falhar, usamos o campo "ret_code" da struct de resposta (não usamos o offset negativo) com o valor ERROR para indicar
ao processo de aplicação que sua operação falhou.

## interControllerSim

Dorme a quantidade de tempo referente à time slice e envia uma interrupção IRQ0 no FIFO de interrupções. Também
envia IRQ1 e IRQ2 a depender do resultado do número aleatório.

## A.c

Acessa o estado da CPU simulada usando shmem e utiliza o contador PC dele para contar suas iterações.
Faz syscalls aleatórias de tempos em tempos, com seus parâmetros aleatorizados para providenciar o que
será necessário nas operações de arquivo/diretório. Para esta construção, optamos por permitir que o
processo saiba quais são os arquivos e diretórios existentes, e este é conhecido pois alteramos para
que logo ao inicializar o processo, ele faça uma syscall do tipo DL, e estes dados remanescem no state
para que o processo consiga posteriormente acessar e saber qual foi a informação mais recente dos arquivos
existentes. 
Achamos necessário fazer isso para que não ficássemos dependentes da aleatoriedade de acertar o mesmo nome
duas vezes (pois também geramos o nome de maneira aleatória dentro de um conjunto possível) e que assim a
maior parte das operações será então bem sucedida.
Tendo construído o syscall_args apropriadamente, envia-os no syscall_fifo. Ao final de todas as
iterações (quando chega no MAX), ele faz uma syscall passando a operação EXIT, de forma a avisar o kernel 
que o processo terminou com sucesso.

## Término

Quando todos os filhos já terminaram todas as iterações, o kernel termina também, pois possui um contador
interno de quantos filhos já finalizaram.

## Exemplos:

### Syscalls de Listar Diretório iniciais para preencher o conhecimento dos Ax

```
SFSS rodando na porta 9876...
Kernel: iniciei filho 0 com pid 26546
Kernel: iniciei filho 1 com pid 26547
Kernel: iniciei filho 2 com pid 26548
Kernel: iniciei filho 3 com pid 26549
Kernel: iniciei filho 4 com pid 26550
Kernel: comecei com filho de pid 26546
Processo 26546: começando pra valer
Processo 26546: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Processo 26546: fiz syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: parei filho com pid 26546
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: Enviado UDP REQ (Owner: 1, is_shared: 0, Op: L, Path: )
Kernel: continuei filho com pid 26547
SFSS: REQ recebido do processo 1 com Op=L
SFSS: Listando diretório ./SFSS-root-dir/A1
Processo 26547: começando pra valer
Processo 26547: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Processo 26547: fiz syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: parei filho com pid 26547
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: Enviado UDP REQ (Owner: 2, is_shared: 0, Op: L, Path: )
Kernel: continuei filho com pid 26548
Processo 26548: começando pra valer
Processo 26548: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Processo 26548: fiz syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
SFSS: Listei 3 arquivos/diretórios: doc_testemydirmypath
SFSS: REQ recebido do processo 2 com Op=L
SFSS: Listando diretório ./SFSS-root-dir/A2
Kernel: parei filho com pid 26548
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: Enviado UDP REQ (Owner: 3, is_shared: 0, Op: L, Path: )
Kernel: continuei filho com pid 26549
Kernel: parei filho com pid 26549
Kernel: Recebi UDP REP (Owner: 1, Ret: 1)
Kernel: continuei filho com pid 26549
Processo 26549: começando pra valer
Processo 26549: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Processo 26549: fiz syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: parei filho com pid 26549
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: Enviado UDP REQ (Owner: 4, is_shared: 0, Op: L, Path: )
Kernel: continuei filho com pid 26550
SFSS: Listei 4 arquivos/diretórios: doc_logfoto_logmydirmypath
SFSS: REQ recebido do processo 3 com Op=L
SFSS: Listando diretório ./SFSS-root-dir/A3
Kernel: parei filho com pid 26550
Kernel: Recebi UDP REP (Owner: 2, Ret: 1)
Kernel: continuei filho com pid 26550
Processo 26550: começando pra valer
Processo 26550: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Processo 26550: fiz syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: parei filho com pid 26550
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='L', payload=''
Kernel: Enviado UDP REQ (Owner: 5, is_shared: 0, Op: L, Path: )
Kernel: o filho 26550 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: Listei 5 arquivos/diretórios: dados_loglog_fotomydirmypathteste_doc
SFSS: REQ recebido do processo 4 com Op=L
SFSS: Listando diretório ./SFSS-root-dir/A4
Kernel: Recebi UDP REP (Owner: 3, Ret: 1)
SFSS: Listei 2 arquivos/diretórios: mydirmypath
SFSS: REQ recebido do processo 5 com Op=L
SFSS: Listando diretório ./SFSS-root-dir/A5
Kernel: Recebi UDP REP (Owner: 4, Ret: 1)
SFSS: Listei 4 arquivos/diretórios: foto_dadoslog_docmydirmypath
Kernel: Recebi UDP REP (Owner: 5, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
```
### Exemplo do relatório da pausa manual

```
Pausa manual:
Vamos inspecionar o estado das coisas:

Estado atual da cpu:
State {
                pid: -1
                PC: -1
                current state: IDLE
                syscall_args: is_shared=0, offset=0, path=, op=N, payload=nZn6XGindnnCWYOH
                is_running: false
                qt_syscalls: -1
                done: true
        }

---------- Ready Queue start    ----------
---------- Ready Queue end      ----------
---------- IRQ 1 IO Queue start ----------
State {
                pid: 27293
                PC: 1
                current state: WAITING_FOR_IO
                syscall_args: is_shared=0, offset=80, path=doc_teste, op=W, payload=nZn6XGindnnCWYOH
                is_running: false
                qt_syscalls: 2
                done: false
        }
---------- IRQ 1 IO Queue end   ----------
---------- IRQ 2 IO Queue start ----------
State {
                pid: 27294
                PC: 0
                current state: WAITING_FOR_IO
                syscall_args: is_shared=0, offset=0, path=, op=L, payload=
                is_running: false
                qt_syscalls: 1
                done: false
        }
State {
                pid: 27295
                PC: 0
                current state: WAITING_FOR_IO
                syscall_args: is_shared=0, offset=0, path=, op=L, payload=
                is_running: false
                qt_syscalls: 1
                done: false
        }
State {
                pid: 27296
                PC: 0
                current state: WAITING_FOR_IO
                syscall_args: is_shared=0, offset=0, path=, op=L, payload=
                is_running: false
                qt_syscalls: 1
                done: false
        }
State {
                pid: 27297
                PC: 0
                current state: WAITING_FOR_IO
                syscall_args: is_shared=0, offset=0, path=, op=L, payload=
                is_running: false
                qt_syscalls: 1
                done: false
        }
---------- IRQ 2 IO Queue end   ----------
---------- File Response Queue start    ----------
Response {
        owner: 1
        ret_code: 1
        offset: 80
        payload: 
        path: 
        allfilenames: 
        nrnames: 0
        }
---------- File Response Queue end      ----------
---------- Dir Response Queue start     ----------
Response {
        owner: 2
        ret_code: 1
        offset: 0
        payload: 
        path: 
        allfilenames: mydirmypath
        nrnames: 2
        }
Response {
        owner: 3
        ret_code: 1
        offset: 0
        payload: 
        path: 
        allfilenames: mydirmypath
        nrnames: 2
        }
Response {
        owner: 4
        ret_code: 1
        offset: 0
        payload: 
        path: 
        allfilenames: mydirmypath
        nrnames: 2
        }
Response {
        owner: 5
        ret_code: 1
        offset: 0
        payload: 
        path: 
        allfilenames: mydirmypath
        nrnames: 2
        }
---------- Dir Response Queue end       ----------
```
### Exemplo de Syscall de Write realmente escrevendo no arquivo já existente

> *conteúdo do arquivo 'mypath' antes da syscall de Write:*
```
aaaaaaaaaaaaaaaa
```

```
Kernel: continuei filho com pid 27294
Processo 27294: vou fazer syscall, com args: is_shared=0, offset=16, path='mypath', op='W', payload='Y8ke8n16yL9cE8hx'
Processo 27294: fiz syscall, com args: is_shared=0, offset=16, path='mypath', op='W', payload='Y8ke8n16yL9cE8hx'
Kernel: parei filho com pid 27294
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=16, path='mypath', op='W', payload='Y8ke8n16yL9cE8hx'
Kernel: Enviado UDP REQ (Owner: 2, is_shared: 0, Op: W, Path: mypath)
Kernel: o filho 27294 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 2 com Op=W
SFSS: Escrevendo arquivo ./SFSS-root-dir/A2/mypath com offset 16
Kernel: Recebi UDP REP (Owner: 2, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
```


> *conteúdo do arquivo 'mypath' depois da syscall de Write:*

```
aaaaaaaaaaaaaaaaY8ke8n16yL9cE8hx
```

### Exemplo de Syscall de Write realmente escrevendo no arquivo e preenchendo com 0x20 (whitespace) o espaço entre o antigo fim do arquivo e o novo offset

> *conteúdo do arquivo 'mypath' antes da syscall de Write:*
```
aaaaaaaaaaaaaaaa
```

```
Kernel: continuei filho com pid 27293
Processo 27293: vou fazer syscall, com args: is_shared=0, offset=32, path='mypath', op='W', payload='Wnkacx4X3ca5nmx9'
Processo 27293: fiz syscall, com args: is_shared=0, offset=32, path='mypath', op='W', payload='Wnkacx4X3ca5nmx9'
Kernel: parei filho com pid 27293
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=32, path='mypath', op='W', payload='Wnkacx4X3ca5nmx9'
Kernel: Enviado UDP REQ (Owner: 1, is_shared: 0, Op: W, Path: mypath)
Kernel: o filho 27293 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 1 com Op=W
SFSS: Escrevendo arquivo ./SFSS-root-dir/A1/mypath com offset 32
Kernel: Recebi UDP REP (Owner: 1, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
```
> *conteúdo do arquivo 'mypath' depois da syscall de Write:*
```
aaaaaaaaaaaaaaaa                Wnkacx4X3ca5nmx9
```

### Exemplo de Syscall de Create Dir criando um diretório na parte compartilhada (A0)

```     
Kernel: continuei filho com pid 27296
Processo 27296: vou fazer syscall, com args: is_shared=1, offset=0, path='mydir', op='C', payload=''
Processo 27296: fiz syscall, com args: is_shared=1, offset=0, path='mydir', op='C', payload=''
Kernel: parei filho com pid 27296
Kernel: processo anterior fez syscall, com args: is_shared=1, offset=0, path='mydir', op='C', payload=''
Kernel: Enviado UDP REQ (Owner: 4, is_shared: 1, Op: C, Path: mydir)
Kernel: o filho 27296 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 4 com Op=C
SFSS: Criando diretório ./SFSS-root-dir/A0/mydir/teste_foto
Kernel: Recebi UDP REP (Owner: 4, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
```
### Exemplo de Syscall de Create Dir tendo falha pois o diretório já existe

```
Kernel: continuei filho com pid 35757
Processo 35757: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='C', payload=''
Processo 35757: fiz syscall, com args: is_shared=0, offset=0, path='', op='C', payload=''
Kernel: parei filho com pid 35757
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='C', payload=''
Kernel: Enviado UDP REQ (Owner: 2, is_shared: 0, Op: C, Path: )
Kernel: o filho 35757 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
SFSS: REQ recebido do processo 2 com Op=C
SFSS: Criando diretório ./SFSS-root-dir/A2/teste_doc
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
Kernel: Recebi UDP REP (Owner: 2, Ret: 1)
[...]
Processo 35757: vou fazer syscall, com args: is_shared=0, offset=0, path='', op='C', payload=''
Kernel: parei filho com pid 35757
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='', op='C', payload=''
Kernel: Enviado UDP REQ (Owner: 2, is_shared: 0, Op: C, Path: )
SFSS: REQ recebido do processo 2 com Op=C
Kernel: o filho 35757 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: Criando diretório ./SFSS-root-dir/A2/teste_doc
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
SFSS: Erro ao criar diretório: File exists
Kernel: Recebi UDP REP (Owner: 2, Ret: -1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
```
> *Perceba que o diretório 'teste_doc' já existia, então a segunda tentativa de criação retornou o ret_code = -1, que indica erro.*

### Exemplo de arquivo sendo deletado por syscall de Delete

```
Kernel: continuei filho com pid 27295
Processo 27295: vou fazer syscall, com args: is_shared=0, offset=0, path='mypath', op='D', payload=''
Processo 27295: fiz syscall, com args: is_shared=0, offset=0, path='mypath', op='D', payload=''
Kernel: parei filho com pid 27295
Kernel: processo anterior fez syscall, com args: is_shared=0, offset=0, path='mypath', op='D', payload=''
Kernel: Enviado UDP REQ (Owner: 3, is_shared: 0, Op: D, Path: mypath)
Kernel: o filho 27295 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 3 com Op=D
SFSS: Deletando arquivo/diretório ./SFSS-root-dir/A3/mypath
Kernel: Recebi UDP REP (Owner: 3, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.

```

> *de fato foi deletado.*

### Syscall de Delete seguido de Read (erro) pois o arquivo foi deletado

```
Processo 9282: vou fazer syscall, com args: is_shared=1, offset=0, path='mypath', op='D', payload=''
Processo 9282: fiz syscall, com args: is_shared=1, offset=0, path='mypath', op='D', payload=''
Kernel: parei filho com pid 9282
Kernel: processo anterior fez syscall, com args: is_shared=1, offset=0, path='mypath', op='D', payload=''
Kernel: Enviado UDP REQ (Owner: 3, is_shared: 1, Op: D, Path: mypath)
Kernel: o filho 9282 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 3 com Op=D
SFSS: Deletando arquivo/diretório ./SFSS-root-dir/A0/mypath
Kernel: Recebi UDP REP (Owner: 3, Ret: 1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
[...]
Processo 9281: vou fazer syscall, com args: is_shared=1, offset=48, path='mypath', op='R', payload=''
Processo 9281: fiz syscall, com args: is_shared=1, offset=48, path='mypath', op='R', payload=''
Kernel: parei filho com pid 9281
Kernel: processo anterior fez syscall, com args: is_shared=1, offset=48, path='mypath', op='R', payload=''
Kernel: Enviado UDP REQ (Owner: 2, is_shared: 1, Op: R, Path: mypath)
Kernel: o filho 9281 acabou de fazer uma syscall, mas era o único executando. Vou ter que deixar a cpu parada
SFSS: REQ recebido do processo 2 com Op=R
SFSS: Lendo arquivo ./SFSS-root-dir/A0/mypath com offset 48
SFSS: Erro ao abrir arquivo: No such file or directory
Kernel: Recebi UDP REP (Owner: 2, Ret: -1)
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.
Kernel: recebi IRQ1 sem ninguém rodando. Vou procurar a resposta para o filho com pid 9281
Kernel: encontrei a resposta para o filho com pid 9281. Agora vou liberá-lo
Kernel: continuei filho com pid 9281
Processo 9281: recebi resposta: ret_code=-1. A operação falhou.
Processo 9281: acabei iteração 4
```