#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum
{
    MAX_SLEEP = 3,
    MIN_SLEEP = 1,
    MAX_NUM = 100,
    MIN_NUM = 1
};

typedef struct resultado
{
    int valor;
    int seq;
} Resultado;

int main(int argc, char *argv[])
{

    int shmid = atoi(argv[1]);
    srand(shmid);

    printf("Filho: acabei de nascer, recebi shmid: %d\n", shmid);

    Resultado *mem = (Resultado *)shmat(shmid, 0, 0);

    sleep((rand() % (MAX_SLEEP + 1 - MIN_SLEEP)) + MIN_SLEEP);

    mem->valor = (rand() % (MAX_NUM + 1 - MIN_NUM)) + MIN_NUM;
    printf("schmid %d: Meu número é %d\n", shmid, mem->valor);
    mem->seq = 1;

    shmdt(mem);
    exit(0);
}