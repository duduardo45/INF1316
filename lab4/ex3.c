#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int nDadosTx, nDadosRx; // quantidade de dados transmitidos/recebidos
    int fd[2]; // descritor dos pipes

    pid_t pid;

    if (pipe(fd) < 0)
    {
    puts ("Erro ao abrir os pipes");
    exit (-1);
    }

    pid = fork();
    if (pid < 0) exit(1);
    if (pid == 0) {
        // filho
        close(fd[0]);
        close(1);
        dup(fd[1]);
        execlp("ps", "ps", NULL);
    }
    else if (pid > 0) {
        // pai
        close(fd[1]);
        close(0);
        dup(fd[0]);
        execlp("wc", "wc", NULL);
    }

    return 0;
}