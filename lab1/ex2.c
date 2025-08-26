#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int mypid, pid, status;

    int var = 1;

    printf("Antes: %d\n", var);

    pid = fork();
    if (pid != 0)
    { // Pai
        waitpid(-1, &status, 0);
        printf("Pai: %d\n", var);
    }
    else
    { // Filho
        var = 5;
        printf("Filho: %d\n", var);
        exit(0);
    }
    return 0;
}