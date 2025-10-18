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

/**
prev_state should NOT be NULL.

if cpu_state_pointer->pid == -1, then we are transitioning FROM idle CPU (should just start the new process)

if dest_state is NULL, then we are transitioning TO idle CPU (should just stop the current process)

otherwise, then we are context switching between two processes (should stop the current one and start the new
one)
*/
void switch_context(State *prev_state, State *cpu_state_pointer, State *dest_state, int was_syscall,
                    int syscall_fifo_fd)
{
    if ((cpu_state_pointer->pid == -1 && dest_state == NULL) || prev_state == NULL)
    {
        printf("Argumentos de switch_context não fazem sentido\n");
        exit(EXIT_FAILURE);
    }

    if (cpu_state_pointer->pid != -1) // we are NOT transitioning FROM idle cpu
    {
        kill(cpu_state_pointer->pid, SIGSTOP);
        printf("Kernel: parei filho com pid %d\n", cpu_state_pointer->pid);

        if (was_syscall)
        {
            syscall_args args;
            read(syscall_fifo_fd, &args, sizeof(args));
            cpu_state_pointer->current_syscall = args;
            cpu_state_pointer->current = WAITING_FOR_IO;
            cpu_state_pointer->qt_syscalls++;
            printf("Kernel: processo anterior fez syscall, com args: device=%d e op=%c\n", args.Dx, args.Op);
        }
        else
        {
            cpu_state_pointer->current = READY;
            printf("Kernel: processo anterior chegou ao fim da fatia de tempo\n");
        }

        cpu_state_pointer->is_running = 0;

        *prev_state = *cpu_state_pointer; // save cpu state so we can resume this process in the future
    }

    if (dest_state != NULL) // we are NOT transitioning TO idle cpu
    {
        *cpu_state_pointer = *dest_state; // change cpu state to hold the new process
        cpu_state_pointer->current = RUNNING;
        cpu_state_pointer->is_running = 1;
        kill(cpu_state_pointer->pid, SIGCONT);
        printf("Kernel: continuei filho com pid %d\n", cpu_state_pointer->pid);
    }
    else // we ARE transitioning TO idle CPU
    {
        cpu_state_pointer->pid = -1;
    }
}

/**
returns the state of the first ready process, NULL if there is no ready process
 */
State *find_first_ready_process_state(State process_states[], int start_idx)
{
    int i = start_idx;

    i++;
    i %= NUM_APP_PROCESSES;

    while (i != start_idx)
    {
        if (process_states[i].current == READY)
        {
            return &process_states[i];
        }
        i++;
        i %= NUM_APP_PROCESSES;
    }

    return NULL;
}

/**
returns the index of the process with a specific pid in process_states, fails fast if not found
 */
int find_idx_from_pid(pid_t pid, State process_states[])
{
    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        if (process_states[i].pid == pid)
        {
            return i;
        }
    }

    exit(EXIT_FAILURE);
    return -1;
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
        perror("Não consegui pegar shmem");
        exit(EXIT_FAILURE);
    }

    State *state = (State *)shmat(shmid, 0, 0);

    // initializing core states of all processes

    State process_states[NUM_APP_PROCESSES];

    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        process_states[i].pid = -1; // -1 for now, will be set after forks
        process_states[i].PC = 0;
        process_states[i].current = READY;
        syscall_args current_syscall = {NO_DEVICE, NO_OPERATION};
        process_states[i].current_syscall = current_syscall;
        process_states[i].is_running = 0;
        process_states[i].qt_syscalls = 0;
        process_states[i].done = 0;
    }

    // creating application processes

    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork deu errado");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        { // child
            char *argv[] = {"A", NULL};
            execv("./build/A", argv);
            perror("Não dei exec!");
            exit(EXIT_FAILURE);
        }
        else
        { // parent
            kill(pid, SIGSTOP);
        }

        process_states[i].pid = pid;
        printf("Kernel: iniciei filho %d com pid %d\n", i, pid);
    }

    // load first process

    *state = process_states[0];

    (*state).current = RUNNING;
    (*state).is_running = 1;

    kill(state->pid, SIGCONT);

    printf("Kernel: comecei com filho de pid %d\n", state->pid);

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

            // TODO: there's a race condition somewhere that's causing the kernel to assign the syscall to the wrong
            // process. should we send the pid as syscall arg to solve this?
            if (FD_ISSET(syscall_fifo_fd, &readfds)) // someone made a syscall
            {
                if (state->pid == -1) // cpu is idle
                {
                    printf("Kernel: situação impossível: cpu está idle e acabei de receber uma syscall!\n");
                    exit(EXIT_FAILURE);
                }

                int current_idx = find_idx_from_pid(state->pid, process_states);

                State *ready_process = find_first_ready_process_state(process_states, current_idx);

                if (ready_process == NULL)
                {
                    printf("Kernel: o filho %d acabou de fazer uma syscall, mas era o único executando. Vou ter que "
                           "deixar a cpu parada\n",
                           state->pid);
                    switch_context(&process_states[current_idx], state, NULL, 1, syscall_fifo_fd);
                }
                else
                {
                    switch_context(&process_states[current_idx], state, ready_process, 1, syscall_fifo_fd);
                }
            }

            if (FD_ISSET(irq_fifo_fd, &readfds)) // some irq came
            {
                enum irq_type buffer;
                read(irq_fifo_fd, &buffer, sizeof(enum irq_type));

                if (buffer == IRQ0) // time slice interrupt
                {
                    if (state->pid == -1) // cpu is idle
                    {
                        printf("Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.\n");
                    }
                    else
                    {
                        int current_idx = find_idx_from_pid(state->pid, process_states);

                        State *ready_process = find_first_ready_process_state(process_states, current_idx);
                        if (ready_process == NULL)
                        {
                            printf("Kernel: o filho com pid %d é o único executando, vou deixar continuar mesmo tendo "
                                   "acabado a fatia de tempo\n",
                                   state->pid);
                        }
                        else
                        {
                            switch_context(&process_states[current_idx], state, ready_process, 0, syscall_fifo_fd);
                        }
                    }
                }
                else if (buffer == IRQ1) // device 1 I/O interrupt
                {
                    // switch_context(State *prev_state, State *cpu_state_pointer, State *dest_state, int was_syscall,
                    // int syscall_fifo_fd)
                }
                else if (buffer == IRQ2) // device 2 I/O interrupt
                {
                    // switch_context(State *prev_state, State *cpu_state_pointer, State *dest_state, int was_syscall,
                    // int syscall_fifo_fd)
                }
                else
                {
                    printf("Kernel: interrupção inválida\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        else
        {
            printf("Nada aconteceu em 5 segundos, deve ter dado problema.\n");
        }
    }
}
