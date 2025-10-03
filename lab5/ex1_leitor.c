#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fifo = open("ex1_fifo", O_WRONLY);
    char texto_lido[101];
    while (1)
    {
        int scanned = scanf(" %[^\n]100s", texto_lido);
        printf("Li\n");
        if (scanned < 0)
        {
            perror("erro ao escanear");
        }
        write(fifo, texto_lido, sizeof(texto_lido));
        printf("Escrevi no fifo\n");
    }

    return 0;
}