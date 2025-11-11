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
    char buffer_for_size[BUFFER_SIZE];

    char *buffer;

    int shmid = shmget(SHMEM_KEY, sizeof(buffer_for_size),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    buffer = (char *)shmat(shmid, 0, 0);

    if (fork() == 0)
    { // Impressor
        semId = semget(SEM_KEY, 1, 0666 | IPC_CREAT);

        setSemValue(semId);
        printf("Impressor: inicializei o semáforo\n");

        sleep(2);

        printf("Impressor: vou começar\n");

        while (1)
        {
            semaforoP(semId);
            if (buffer[BUFFER_SIZE - 1] != -1)
            {
                printf("Impressor: opa, buffer cheio!\n");
                for (int i = 0; i < BUFFER_SIZE; i++)
                {
                    putchar(buffer[i]);
                    fflush(stdout);
                    buffer[i] = -1;
                }
                printf("\n");
                printf("Impressor: resetei buffer e imprimi\n");
            }

            printf("Impressor: indo dormir com semáforo...\n");

            sleep(rand() % 3);

            semaforoV(semId);

            printf("Impressor: indo dormir sem semáforo...\n");
            sleep(rand() % 2);
        }
    }
    else
    { // Leitor
        signal(SIGINT, intHandler);

        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            buffer[i] = -1;
        }

        printf("Leitor: inicializei o buffer\n");

        while ((semId = semget(SEM_KEY, 1, 0666)) < 0)
        {
            putchar('.');
            fflush(stdout);
            sleep(1);
        }

        printf("Leitor: peguei o semáforo\n");

        char my_char = -1;

        while (1)
        {
            if (my_char == -1)
            {
                printf("Leitor: não tinha caracter, então vou ler da tela\n");
                while ((my_char = getchar()) == 10) // pular enter
                {
                };
                printf("Leitor: li '%c' (%d)\n", my_char, my_char);
            }
            else
            {
                printf("Leitor: tinha caracter '%c', então não faço nada\n", my_char);
            }

            semaforoP(semId);

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                if (buffer[i] == -1)
                {
                    buffer[i] = my_char;
                    printf("Leitor: achei espaço vazio em %d, botei '%c' (%d) nele\n", i, my_char, my_char);
                    my_char = -1;
                    break;
                }
                else if (i == BUFFER_SIZE - 1)
                {
                    printf("Leitor: não achei espaço vazio, vou guardar '%c'\n", my_char);
                }
            }

            printf("Leitor: indo dormir com semáforo...\n");

            sleep(rand() % 3);
            semaforoV(semId);

            printf("Leitor: indo dormir sem semáforo...\n");
            sleep(rand() % 2);
        }
    }
    if (argc > 1)
    {
        sleep(10);
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