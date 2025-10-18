/*

pseudo código:

first {
    define {
        D1_queue (IRQ_queue)
        D2_queue (IRQ_queue)
        ready_queue
        time_slice
        running
    }
    open core_state_sim (shared memory)
    create A1 : A5
    stop A1 : A5
    init A1 : A5 core_state

    move A2 : A5 to ready_queue
    move A1 to running
    start A1
}

when (IRQ) {
    check IRQ_queue
    if (P was waiting) {
        move P to bottom of ready_queue
    }
}

when (syscall) {
    stop running
    save core_state_sim
    move running to IRQ_queue
    move top of ready_queue to running
    load running core_state
    start running
}

when (manual pause) {
    if (not paused) {
        stop running
        set paused
        dump running core_state
        print time_slice_remaining
        list ready_queue
        list D1_queue
        list D2_queue
    }
}

when (manual unpause) {
    if (paused) {
        unset paused
        start running
    }
}

while (not paused) {

}

*/

#include "constants.h"
#include "state.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_APP_PROCESSES 5

void create_read_fifo(char path[])
{
    int fifo_done = mkfifo(path, 0666);

    if (fifo_done < 0)
    {
        if (errno != EEXIST)
        {
            perror("Não consegui criar fifo.");
            exit(EXIT_FAILURE);
        }
    }
}

int main(void)
{
    create_read_fifo(IRQ_FIFO_PATH);
    create_read_fifo(SYSCALL_FIFO_PATH);

    // start interController process

    pid_t inter_pid = fork();

    if (inter_pid == 0)
    {
        char *argv[] = {"interControllerSim", NULL};
        execv("./build/interControllerSim", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }

    int irq_fifo_fd =
        open(IRQ_FIFO_PATH, O_RDONLY); // only opening after fork because other process needs to open as well

    if (irq_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }

    // init core state shmem

    int shmid = shmget(CORE_STATE_SHMEM_KEY, sizeof(State),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    State *state = (State *)shmat(shmid, 0, 0);

    // creating application processes

    pid_t pids[NUM_APP_PROCESSES];

    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            perror("Fork deu errado");
            exit(EXIT_FAILURE);
        }
        else if (pids[i] == 0)
        { // child
            char *argv[] = {"A", NULL};
            execv("./build/A", argv);
            perror("Não dei exec!");
            exit(EXIT_FAILURE);
        }
        else
        { // parent
            kill(pids[i], SIGSTOP);
        }
    }

    // initializing core states of all processes

    State core_states[NUM_APP_PROCESSES];

    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        core_states[i].PC = 0;
        core_states[i].current = READY;
        core_states[i].blocked_by_device = 0;
        syscall_args current_syscall = {0, 0};
        core_states[i].current_syscall = current_syscall;
        core_states[i].is_running = 0;
        core_states[i].qt_syscalls = 0;
        core_states[i].done = 0;
    }

    // load first process

    int running_child = 0;

    *state = core_states[running_child];

    (*state).current = RUNNING;
    (*state).is_running = 1;

    kill(pids[running_child], SIGCONT);

    printf("Kernel: continuei filho %d com pid %d\n", running_child, pids[running_child]);

    // only opening after fork because other process needs to open as well
    int syscall_fifo_fd = open(SYSCALL_FIFO_PATH, O_RDONLY);

    if (syscall_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }

    // preparing some select args

    int max_fd = (irq_fifo_fd >= syscall_fifo_fd) ? irq_fifo_fd : syscall_fifo_fd;

    struct timespec timeout;

    timeout.tv_sec = 5;
    timeout.tv_nsec = 0;

    // normal operation starts

    while (1)
    {
        // using select in order to "keep up with" both irq and syscall file descriptors at the same time

        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(irq_fifo_fd, &readfds);
        FD_SET(syscall_fifo_fd, &readfds);

        int num_ready = pselect(max_fd + 1, &readfds, NULL, NULL, &timeout, NULL);

        if (num_ready == -1)
        {
            perror("select()");
        }
        else if (num_ready) // either irq or syscall came
        {
            if (FD_ISSET(irq_fifo_fd, &readfds)) // some irq came
            {
                enum irq_type buffer;
                read(irq_fifo_fd, &buffer, sizeof(enum irq_type));

                if (buffer == IRQ0) // time slice interrupt
                {
                    kill(pids[running_child], SIGSTOP);
                    printf("Kernel: parei filho %d com pid %d\n", running_child, pids[running_child]);

                    (*state).current = READY;
                    (*state).is_running = 0;

                    core_states[running_child] = *state;

                    running_child++;
                    running_child %= NUM_APP_PROCESSES;

                    *state = core_states[running_child];

                    (*state).current = RUNNING;
                    (*state).is_running = 1;

                    kill(pids[running_child], SIGCONT);
                    printf("Kernel: continuei filho %d com pid %d\n", running_child, pids[running_child]);
                }
                else if (buffer == IRQ1) // device 1 I/O interrupt
                {
                    // TODO: implement
                }
                else // device 2 I/O interrupt
                {
                }
            }

            if (FD_ISSET(syscall_fifo_fd, &readfds)) // someone made a syscall
            {
                kill(pids[running_child], SIGSTOP);
                printf("Kernel: parei filho %d com pid %d porque fez syscall\n", running_child, pids[running_child]);

                syscall_args args;
                read(syscall_fifo_fd, &args, sizeof(args));
                printf("Kernel: syscall com args: device=%d e op=%c\n", args.Dx, args.Op);

                (*state).current = READY;
                (*state).is_running = 0;

                core_states[running_child] = *state;

                running_child++;
                running_child %= NUM_APP_PROCESSES;

                *state = core_states[running_child];

                (*state).current = RUNNING;
                (*state).is_running = 1;

                kill(pids[running_child], SIGCONT);
                printf("Kernel: continuei filho %d com pid %d\n", running_child, pids[running_child]);
            }
        }

        else
        {
            printf("Nada aconteceu em 5 segundos, deve ter dado problema.\n");
        }
    }
}
