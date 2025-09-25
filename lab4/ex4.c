#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_MESSAGE_SIZE 50

int main(int argc, char *argv[])
{
    int fd[2]; // descritor dos pipes

    char textoTX[MAX_MESSAGE_SIZE] = "1";
    char textoRX[sizeof textoTX];

    ssize_t nDadosTx = 0;
    ssize_t nDadosRx = 0;

    pid_t pid = -1;

    if (pipe(fd) < 0)
    {
        puts("Erro ao abrir os pipes");
        exit(-1);
    }

    pid = fork();
    if (pid < 0)
        exit(1);
    if (pid == 0)
    {
        // filho 1
        close(fd[1]);
        while (1)
        {
            sleep(1);
            nDadosRx = read(fd[0], textoRX, sizeof(textoRX));
            printf("Filho 1: li %ld dados: %s\n", nDadosRx, textoRX);
        }
    }

    pid = fork();
    if (pid < 0)
        exit(1);
    if (pid == 0)
    {
        // filho 2
        close(fd[1]);
        while (2)
        {
            sleep(1);
            nDadosRx = read(fd[0], textoRX, sizeof(textoRX));
            printf("Filho 2: li %ld dados: %s\n", nDadosRx, textoRX);
        }
    }

    else if (pid > 0)
    {
        // pai
        close(fd[0]);
        int i = 1;
        while (1)
        {
            nDadosTx = write(fd[1], textoTX, sizeof(textoTX) + 1);
            printf("Pai: escrevi %ld dados: %s\n", nDadosTx, textoTX);
            i++;
            char i_str[MAX_MESSAGE_SIZE] = "";
            (void)sprintf(i_str, "%d", i);
            strcat(textoTX, i_str);
            sleep(2);
        }
    }

    return 0;
}