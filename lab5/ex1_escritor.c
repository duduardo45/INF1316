#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fifo = open("ex1_fifo", O_RDONLY);
    char texto_pra_escrever[101];
    while (1)
    {
        int fifo_read = read(fifo, texto_pra_escrever, sizeof(texto_pra_escrever));
        if (fifo_read < 0)
        {
            perror("Erro lendo do fifo");
        }
        printf("%s\n", texto_pra_escrever);
    }

    return 0;
}