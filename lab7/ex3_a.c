#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHMEM_KEY 66421
#define SEM_KEY 8752
#define BUFFER_SIZE 16
#define VALOR_SOMA 1

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int semId;

// inicializa o valor do semáforo
int setSemValue(int semId);
// remove o semáforo
void delSemValue(int semId);
// operação P
int semaforoP(int semId);
// operação V
int semaforoV(int semId);

void intHandler(int signal)
{
    printf("Vou parar, espera um tiquinho...\n");

    sleep(5);

    delSemValue(semId);

    exit(0);
}

int main(int argc, char *argv[])
{
    long *var;

    int shmid = shmget(SHMEM_KEY, sizeof(var), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    var = (long *)shmat(shmid, 0, 0);

    if (fork() == 0)
    { // Programa B
        char *argv[] = {"ex3_b", NULL};
        execv("./build/ex3_b", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }
    else
    { // Programa A
        signal(SIGINT, intHandler);

        *var = 0;

        printf("A: inicializei a var\n");

        while ((semId = semget(SEM_KEY, 1, 0666)) < 0)
        {
            putchar('.');
            fflush(stdout);
            sleep(1);
        }

        while (1)
        {
            printf("A: vou pegar semáforo...\n");
            semaforoP(semId);
            printf("A: peguei semáforo!\n");

            printf("A: o valor tá em %ld...\n", *var);
            *var += VALOR_SOMA;
            printf("A: e foi pra %ld\n", *var);

            printf("A: indo dormir com semáforo...\n");
            sleep(rand() % 3);

            printf("A: vou devolver semáforo...\n");
            semaforoV(semId);

            printf("A: indo dormir sem semáforo...\n");
            sleep(rand() % 2);
        }
    }

    return 0;
}

int setSemValue(int semId)
{
    union semun semUnion;
    semUnion.val = 1;
    return semctl(semId, 0, SETVAL, semUnion);
}
void delSemValue(int semId)
{
    union semun semUnion;
    semctl(semId, 0, IPC_RMID, semUnion);
}
int semaforoP(int semId)
{
    struct sembuf semB;
    semB.sem_num = 0;
    semB.sem_op = -1;
    semB.sem_flg = SEM_UNDO;
    semop(semId, &semB, 1);
    return 0;
}
int semaforoV(int semId)
{
    struct sembuf semB;
    semB.sem_num = 0;
    semB.sem_op = 1;
    semB.sem_flg = SEM_UNDO;
    semop(semId, &semB, 1);
    return 0;
}