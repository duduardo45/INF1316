

# Exemplos:

### Syscalls de Listar Diretório iniciais para preencher o conhecimento dos Ax

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

### Exemplo do relatório da pausa manual

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

### Exemplo de Syscall de Write realmente escrevendo no arquivo já existente

> *conteúdo do arquivo 'mypath' antes da syscall de Write:*
```
aaaaaaaaaaaaaaaa
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


> *conteúdo do arquivo 'mypath' depois da syscall de Write:*
```
aaaaaaaaaaaaaaaaY8ke8n16yL9cE8hx
```

### Exemplo de Syscall de Write realmente escrevendo no arquivo e preenchendo com 0x20 (whitespace) o espaço entre o antigo fim do arquivo e o novo offset

> *conteúdo do arquivo 'mypath' antes da syscall de Write:*
```
aaaaaaaaaaaaaaaa
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

> *conteúdo do arquivo 'mypath' depois da syscall de Write:*
```
aaaaaaaaaaaaaaaa                Wnkacx4X3ca5nmx9
```

### Exemplo de Syscall de Create Dir criando um diretório na parte compartilhada (A0)

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

### Exemplo de Syscall de Create Dir tendo falha pois o diretório já existe

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

> *Perceba que o diretório 'teste_doc' já existia, então a segunda tentativa de criação retornou o ret_code = -1, que indica erro.*

### Exemplo de arquivo sendo deletado por syscall de Delete

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

> *de fato foi deletado.*

### Syscall de Delete seguido de Read (erro) pois o arquivo foi deletado

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