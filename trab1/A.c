#include "constants.h"
#include "state.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX 100

void syscall_sim(pid_t mypid, int syscall_fifo, syscall_args args)
{
    printf("Processo %d: vou fazer syscall, com args: device=%d e op=%c\n", mypid, args.Dx, args.Op);
    write(syscall_fifo, &args, sizeof(args));
    printf("Processo %d: fiz syscall, com args: device=%d e op=%c\n", mypid, args.Dx, args.Op);
}

void maybe_syscall(pid_t mypid, int syscall_fifo)
{
    int d = ((rand() % 100) + 1);
    if (d < 15) // generate a random syscall with low probability
    {
        enum operation_type op;

        if ((d % 3) == 1)
        {
            op = R;
        }
        else if ((d % 3) == 2)
        {
            op = W;
        }
        else
        {
            op = X;
        }

        syscall_args args = {
            (d % 2) ? D1 : D2,
            op,
        };

        syscall_sim(mypid, syscall_fifo, args);
    }
}

int main(void)
{
    int syscall_fifo = open(SYSCALL_FIFO_PATH, O_WRONLY);
    if (syscall_fifo < 0)
    {
        perror("erro ao abrir fifo");
    }

    pid_t mypid = getpid();

    srand(time(NULL) ^ getpid());
    int shmid = shmget(CORE_STATE_SHMEM_KEY, sizeof(State),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    State *state = (State *)shmat(shmid, 0, 0);

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = A_SLEEP;

    printf("Processo %d: começando pra valer\n", mypid);

    for (state->PC = 0; state->PC < MAX; state->PC++)
    {
        nanosleep(&tim, &tim2);
        maybe_syscall(mypid, syscall_fifo);
        nanosleep(&tim, &tim2);
        printf("Processo %d: acabei iteração %d\n", mypid, state->PC);
    }
}
