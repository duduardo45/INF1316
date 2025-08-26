#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int mypid, pid, status;
    mypid = getpid();
    pid = fork();
    if (pid != 0)
    { // Pai
        printf("Meu PID (pai): %d\n", mypid);
        waitpid(-1, &status, 0);
    }
    else
    { // Filho
        mypid = getpid();
        printf("Meu PID (filho): %d\n", mypid);
    }
    return 0;
}