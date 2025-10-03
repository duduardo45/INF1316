#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fifo_done = mkfifo("ex2_fifo", 0666);

    if (fifo_done < 0)
    {
        perror("erro ao criar fifo");
    }

    int pids[2];
    for (int i = 0; i < 2; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            int fifo = open("ex2_fifo", O_WRONLY);
            if (fifo < 0)
            {
                perror("erro ao abrir fifo");
            }
            char buffer[] = "Eu sou o filho melhor!";
            printf("vou escrever\n");
            write(fifo, buffer, sizeof(buffer));
            printf("ja escrevi\n");
            close(fifo);
            exit(0);
        }
    }

    int fifo = open("ex2_fifo", O_RDONLY);

    int stat_loc = -1;

    waitpid(pids[0], &stat_loc, 0);
    waitpid(pids[1], &stat_loc, 0);

    char buffer[100];

    ssize_t bytes_read = -1;

    bytes_read = read(fifo, buffer, sizeof(buffer));
    printf("Filho disse:\n%s\n", buffer);

    while (bytes_read > 0)
    {
        bytes_read = read(fifo, buffer, sizeof(buffer));
        printf("Filho disse:\n%s\n", buffer);
    }

    close(fifo);
    unlink("ex2_fifo");

    return 0;
}