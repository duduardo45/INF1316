#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void printa_vetor(char *palavra, int *vetor)
{
    printf("%s\n", palavra);
    for (int i = 0; i < 10; i++)
    {
        printf("%d, ", vetor[i]);
    };
    printf("\n");
}

int compara(const void *A, const void *B)
{
    int a = *((int *)A);
    int b = *((int *)B);

    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

int main(void)
{
    int mypid, pid, status;

    int vetor[] = {1, 4, 5, 2, 8, 7, 3, 6, 10, 9};

    printa_vetor("Antes: ", vetor);

    pid = fork();
    if (pid != 0)
    { // Pai
        waitpid(-1, &status, 0);
        printa_vetor("Pai: ", vetor);
    }
    else
    { // Filho
        qsort(vetor, 10, sizeof(int), compara);
        char *argv[] = {"echo", "uau!", NULL};
        execvp("echo", argv);
        exit(0);
    }
    return 0;
}