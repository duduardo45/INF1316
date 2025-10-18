#include "constants.h"
#include "state.h"
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX 100

void maybe_syscall()
{
    int d = ((rand() % 100) + 1);
    if (d < 15) // generate a random syscall with low probability
    {
        enum operation_type op;

        if ((d % 3) == 1)
        {
            op = R;
        }
        else if ((d % 3) == 1)
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

        // syscall(args); // TODO: temos que escrever a syscall nossa
        printf("Deveria fazer syscall agora, com args: device=%d e op=%d\n", args.Dx, args.Op);
    }
}

int main(void)
{
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
        maybe_syscall();
        nanosleep(&tim, &tim2);
        printf("Processo %d: acabei iteração %d\n", mypid, state->PC);
    }
}
