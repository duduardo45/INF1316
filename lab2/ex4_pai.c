#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct resultado
{
    int valor;
    int seq;
} Resultado;

int main(void)
{
    int shmid_m1 = shmget(IPC_PRIVATE, sizeof(Resultado), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid_m1 < 0)
    {
        printf("Erro ao criar a memoria compartilhada shmid\n");
        return 1;
    }

    int shmid_m2 = shmget(IPC_PRIVATE, sizeof(Resultado), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid_m2 < 0)
    {
        printf("Erro ao criar a memoria compartilhada shmid\n");
        return 1;
    }

    Resultado *m1 = (Resultado *)shmat(shmid_m1, 0, 0);
    Resultado *m2 = (Resultado *)shmat(shmid_m2, 0, 0);

    m1->seq = 0;
    m2->seq = 0;

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        char shmid_m1_str[50];

        int conv = sprintf(shmid_m1_str, "%d", shmid_m1);
        if (conv == -1)
        {
            printf("Erro com conv\n");
            exit(1);
        }
        execl("./build/ex4_filho", "%d", shmid_m1_str, (char *)0);
        printf("Não consegui dar exec\n");
        exit(1);
    }
    else
    {
        printf("Pai: acabei de dar o fork de m1\n");
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        char shmid_m2_str[50];

        int conv = sprintf(shmid_m2_str, "%d", shmid_m2);
        if (conv == -1)
        {
            printf("Erro com conv\n");
            exit(1);
        }
        execl("./build/ex4_filho", "%d", shmid_m2_str, (char *)0);
        printf("Não consegui dar exec\n");
        exit(1);
    }
    else
    {
        printf("Pai: acabei de dar o fork de m2\n");
    }

    while (!(m1->seq && m2->seq))
        ;

    wait(NULL);
    wait(NULL);

    printf("A multiplicação deu: %d\n", m1->valor * m2->valor);

    shmdt(m1);
    shmdt(m2);

    shmctl(shmid_m1, IPC_RMID, 0);
    shmctl(shmid_m2, IPC_RMID, 0);

    return 0;
}
