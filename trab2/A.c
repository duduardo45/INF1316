#include "constants.h"
#include "state.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX 10

void syscall_sim(pid_t mypid, int syscall_fifo, syscall_args args)
{
    printf("Processo %d: vou fazer syscall, com args: is_shared=%d, offset=%c, path=%s, op=%c, \n", mypid,
           args.is_shared, args.offset, args.path, args.Op);
    write(syscall_fifo, &args, sizeof(args));
    printf("Processo %d: fiz syscall, com args: is_shared=%d, offset=%c, path=%s, op=%c\n", mypid, args.is_shared,
           args.offset, args.path, args.Op);
}

/** returns 1 if syscall happened, 0 otherwise */
int maybe_syscall(pid_t mypid, int syscall_fifo)
{
    int d = (rand() % 100);
    if (d < 15) // generate a random syscall with low probability
    {
        enum operation_type op;

        int op_choice =
            (d % 5); // TODO: is this going to have higher probability for some over others? since d is 0 to 14
        switch (op_choice)
        {
        case 0:
            op = WR;
            break;
        case 1:
            op = RD;
            break;
        case 2:
            op = DC;
            break;
        case 3:
            op = DR;
            break;
        case 4:
            op = DL;
            break;
        default:
            printf("Resto inválido\n");
            exit(EXIT_FAILURE);
            break;
        }

        int offset_choice = (d % 7);
        int offset_val = offset_choice * 16;

        char path[] = "/mypath"; // TODO: not hardcode path

        syscall_args args = {
            0, // TODO: not hardcode is_shared
            offset_val,
            path,
            op,
        };

        syscall_sim(mypid, syscall_fifo, args);
        return 1;
    }
    return 0;
}

void print_response(pid_t mypid, State *state)
{
    printf("Processo %d: recebi resposta: ret_code=%d, payload=%s\n", mypid, state->current_response.ret_code,
           state->current_response.payload);

    state->current_response.ret_code = EMPTY;
    strcpy(state->current_response.payload, "");
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
        int syscall_happened = maybe_syscall(mypid, syscall_fifo);
        nanosleep(&tim, &tim2);
        if (syscall_happened)
        {
            print_response(mypid, state);
        }
        printf("Processo %d: acabei iteração %d\n", mypid, state->PC);
    }

    printf("Processo %d: acabei tudo!\n", mypid);

    syscall_args args = {
        NO_DEVICE,
        NO_OPERATION,
    };

    write(syscall_fifo, &args, sizeof(args)); // fake exit syscall

    return 0;
}
