#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int nDadosTx, nDadosRx; // quantidade de dados transmitidos/recebidos
    int fd[2];              // descritor dos pipes

    fd[0] = open("./lab4/entrada.txt", O_RDONLY);
    fd[1] = open("./lab4/saida.txt", O_WRONLY);

    close(0);
    dup(fd[0]);
    close(1);
    dup(fd[1]);

    char buf[100];

    scanf(" %[^\n]s", buf);
    printf("%s", buf);

    return 0;
}