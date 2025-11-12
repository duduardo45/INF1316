#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define SHMEM_KEY 66421
#define SEM_KEY 8752
#define BUFFER_SIZE 64
#define A_EMPTY 0
#define A_FULL 1
#define B_EMPTY 2
#define B_FULL 3
#define NUM_SEMS 4
#define MESSAGE_SIZE (BUFFER_SIZE / 2)

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int semId;

char* sendBuffer;
char* receiveBuffer;

// inicializa o valor do semáforo
int setSemValue(int semId, int semNum, int value);
// remove o semáforo
void delSemValue(int semId);
// operação P
int semaforoP(int semId, int semNum);
// operação V
int semaforoV(int semId, int semNum);

char* sendMessage();

char* receiveMessage();

int raiseProgramB()
{
    if (fork() == 0)
    { // Programa B
        char *argv[] = {"ex4_b", NULL};
        execv("./build/ex4_b", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int programARoutine()
{
    printf("Programa A iniciado.\n");
    semId = semget(SEM_KEY, NUM_SEMS, 0666 | IPC_CREAT);
    if (semId < 0)
    {
        perror("Não consegui criar semáforo.");
        exit(EXIT_FAILURE);
    }

    setSemValue(semId, A_EMPTY, 1);
    setSemValue(semId, A_FULL, 0);
    setSemValue(semId, B_EMPTY, 1);
    setSemValue(semId, B_FULL, 0);

    printf("A: Semáforos inicializados.\n");

    printf("A: Iniciando rotina...\n");
    int nadaFeito = 1;
    while (1)
    {
        if (nadaFeito) {
            printf("A: Nada a fazer, vou dormir...\n");
            sleep(1);
        }

        printf("A: Tentando receber mensagem...\n");
        if (semaforoP(semId, A_FULL) == 0) {
            char* message = receiveMessage();
            printf("A: Mensagem recebida: %s\n", message);
            semaforoV(semId, A_EMPTY);
            nadaFeito = 0;
        } else {
            printf("A: Não consegui receber mensagem.\n");
            nadaFeito = 1;
        }

        printf("A: Tentando enviar mensagem...\n");
        if (semaforoP(semId, B_EMPTY) == 0) {
            char* message = sendMessage();
            printf("A: Enviando mensagem: %s\n", message);
            semaforoV(semId, B_FULL);
            nadaFeito = 0;
        } else {
            printf("A: Não consegui enviar mensagem.\n");
            nadaFeito = 1;
        }

        sleep(1);
    }

    return 0;
}

void intHandler(int signal)
{
    printf("Vou parar, espera um tiquinho...\n");

    sleep(5);

    delSemValue(semId);
    shmdt(sendBuffer);

    exit(0);
}

int main(int argc, char *argv[])
{
    srand((unsigned int)time(NULL));
    signal(SIGINT, intHandler);

    int shmid = shmget(SHMEM_KEY, BUFFER_SIZE, IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    sendBuffer = (char*) shmat(shmid, 0, 0);
    receiveBuffer = sendBuffer + (MESSAGE_SIZE);

    raiseProgramB();
    
    programARoutine();

    return 0;
}



int setSemValue(int semId, int semNum, int value)
{
    union semun semUnion;
    semUnion.val = value;
    return semctl(semId, semNum, SETVAL, semUnion);
}

void delSemValue(int semId)
{
    union semun semUnion;
    semctl(semId, 0, IPC_RMID, semUnion);
}

int semaforoP(int semId, int semNum)
{
    struct sembuf semB;
    semB.sem_num = semNum;
    semB.sem_op = -1;
    semB.sem_flg = SEM_UNDO | IPC_NOWAIT;
    if (semop(semId, &semB, 1) == -1) {
        if (errno != EAGAIN) {
            perror("semop P failed");
            exit(EXIT_FAILURE);
        }
        return -1;
    }
    return 0;
}

int semaforoV(int semId, int semNum)
{
    struct sembuf semB;
    semB.sem_num = semNum;
    semB.sem_op = 1;
    semB.sem_flg = SEM_UNDO;
    return semop(semId, &semB, 1);
}

char* randomMessage() {
    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";
    static char message[MESSAGE_SIZE + 1];

    for (int i = 0; i < MESSAGE_SIZE; ++i) {
        message[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    message[MESSAGE_SIZE] = '\0';
    return message;
}

char* sendMessage() {
    char* message = randomMessage();

    for (int i = 0; i < MESSAGE_SIZE; i++) {
        sendBuffer[i] = message[i];
    }
    return message;
}

char* receiveMessage() {
    static char message[MESSAGE_SIZE + 1];
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        message[i] = receiveBuffer[i];
    }
    message[MESSAGE_SIZE] = '\0';
    return message;
}