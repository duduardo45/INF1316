#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
    VEC_SIZE = 400,
    NUM_PROCESSES = 4,
    NUM_PER_PROCESS = VEC_SIZE / NUM_PROCESSES,
    VALUE_TO_SEARCH = 345
};

int search_value(int *vec, int valor, int start_pos, int end_pos)
{
    for (int i = start_pos; i < end_pos; i++)
    {
        if (vec[i] == valor)
        {
            printf("Procurei, achei %d\n", i);
            return i;
        }
    }
    printf("Procurei nao achei\n");
    return -1;
}

int main(void)
{
    size_t tam = VEC_SIZE;

    int shmid = shmget(IPC_PRIVATE, tam, IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0)
    {
        printf("Erro ao criar a memoria compartilhada shmid\n");
        return 1;
    }

    int *vec = (int *)shmat(shmid, 0, 0);

    int shmid_found_pos = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid_found_pos < 0)
    {
        printf("Erro ao criar a memoria compartilhada shmid_found_pos\n");
        return 1;
    }

    int *pfound_pos = (int *)shmat(shmid, 0, 0);
    for (int i = 0; i < tam; i++)
    {
        vec[i] = VEC_SIZE - i;
    }

    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        pid_t pid = fork();
        if (pid != 0)
        {
            printf("Filho %d: acabei de nascer\n", i);
            int found = search_value(vec, VALUE_TO_SEARCH, i * NUM_PER_PROCESS, (i + 1) * NUM_PER_PROCESS);
            if (found != -1)
            {
                *pfound_pos = found;
            }
            shmdt(vec);
            exit(0);
        }
        else
        {
            printf("Pai: acabei de dar o fork %d\n", i);
        }
    }

    printf("Pai: posição que um filho descobriu: %d\n", *pfound_pos);

    shmctl(shmid, IPC_RMID, 0);
    shmctl(shmid_found_pos, IPC_RMID, 0);
}