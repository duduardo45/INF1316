#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int nDadosTx, nDadosRx; // quantidade de dados transmitidos/recebidos
    int fd[2]; // descritor dos pipes
    const char textoTX[] = "uma mensagem";
    char textoRX[sizeof textoTX];

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
        nDadosTx = write(fd[1], textoTX, strlen(textoTX)+1);
        printf("Filho: %d dados escritos\n", nDadosTx);
        close(fd[1]);
    }
    else if (pid > 0) {
        // pai
        close(fd[1]);
        nDadosRx = read(fd[0], textoRX, sizeof textoRX);
        printf("Pai: %d dados lidos: %s\n", nDadosRx, textoRX);
        close(fd[0]);
    }

    return 0;
}