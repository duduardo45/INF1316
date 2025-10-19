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
    stop running
    save core_state_sim
    if (P was waiting) {
        move P to top of ready_queue
    }
    move top of ready_queue to running
    load running core_state
    start running
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

Queue* ready_queue_start = NULL;
Queue* D1_queue_start = NULL;
Queue* D2_queue_start = NULL;
Queue* ready_queue_end = NULL;
Queue* D1_queue_end = NULL;
Queue* D2_queue_end = NULL;


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
void switch_context(State *prev_state, State *cpu_state_pointer, State *dest_state)
{
    if (cpu_state_pointer->pid == -1 && dest_state == NULL)
    {
        printf("Argumentos de switch_context não fazem sentido\n");
        exit(EXIT_FAILURE);
    }

    if (cpu_state_pointer->pid != -1) // we are NOT transitioning FROM idle cpu
    {
        cpu_state_pointer->is_running = 0;
        if (prev_state != NULL) *prev_state = *cpu_state_pointer; // save cpu state so we can resume this process in the future
    }

    if (dest_state != NULL) // we are NOT transitioning TO idle cpu
    {
        *cpu_state_pointer = *dest_state; // change cpu state to hold the new process
    }
    else // we ARE transitioning TO idle CPU
    {
        cpu_state_pointer->pid = -1;
    }
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

void initialize_fifos(void)
{
    create_read_fifo(IRQ_FIFO_PATH);
    create_read_fifo(SYSCALL_FIFO_PATH);
}

void start_intercontroller(void)
{
    // start interController process
    pid_t inter_pid = fork();

    if (inter_pid == 0)
    {
        char *argv[] = {"interControllerSim", NULL};
        execv("./build/interControllerSim", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }
}

int open_irq_fifo(void)
{
    int irq_fifo_fd =
        open(IRQ_FIFO_PATH, O_RDONLY); // only opening after fork because other process needs to open as well

    if (irq_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }
    return irq_fifo_fd;
}

State* initialize_shared_memory(void)
{
    // init core state shmem
    int shmid = shmget(CORE_STATE_SHMEM_KEY, sizeof(State),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0)
    {
        perror("Não consegui pegar shmem");
        exit(EXIT_FAILURE);
    }

    State *state = (State *)shmat(shmid, 0, 0);
    return state;
}

void initialize_process_states_array(State process_states[])
{
    // initializing core states of all processes
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
}

void create_application_processes(State process_states[])
{
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
}

void load_and_start_first_process(State* state, State process_states[])
{
    // load first process
    *state = process_states[0];
    (*state).current = RUNNING;
    (*state).is_running = 1;

    State *temp;
    // fill the ready queue
    for (int i = 1; i < NUM_APP_PROCESSES; i++)
    {
        temp = &process_states[i];
        (*temp).current = READY;
        (*temp).is_running = 0;
        insert_end(&ready_queue_start, &ready_queue_end, i);
    }

    kill(state->pid, SIGCONT);
    printf("Kernel: comecei com filho de pid %d\n", state->pid);
}

int open_syscall_fifo(void)
{
    // only opening after fork because other process needs to open as well
    int syscall_fifo_fd = open(SYSCALL_FIFO_PATH, O_RDONLY | O_NONBLOCK);

    if (syscall_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }
    return syscall_fifo_fd;
}

void setup_pselect(int irq_fifo_fd, int syscall_fifo_fd, int* max_fd, struct timespec* timeout)
{
    // preparing some select args
    *max_fd = (irq_fifo_fd >= syscall_fifo_fd) ? irq_fifo_fd : syscall_fifo_fd;
    timeout->tv_sec = 5;
    timeout->tv_nsec = 0;
}

void handle_syscall(State* state, State process_states[], int syscall_fifo_fd)
{
    if (state->pid == -1) // cpu is idle
    {
        printf("Kernel: situação impossível: cpu está idle e acabei de receber uma syscall!\n");
        exit(EXIT_FAILURE);
    }

    int current_idx = find_idx_from_pid(state->pid, process_states);
    int ready_process = pop_start(&ready_queue_start, &ready_queue_end);

    syscall_args args;
    // Checks to see if there's a syscall to be processed
    if (read(syscall_fifo_fd, &args, sizeof(args)) == -1) {
        // Unexpected FIFO reading error
        perror("Kernel: erro ao ler syscall_fifo_fd");
        exit(EXIT_FAILURE);
    }

    // syscall handling logic
    state->current_syscall = args;
    state->current = WAITING_FOR_IO;
    state->qt_syscalls++;
    enum device_number device = args.Dx;
    printf("Kernel: processo anterior fez syscall, com args: device=%d e op=%c\n", device, args.Op);

    if (device == D1) {
        insert_end(&D1_queue_start, &D1_queue_end, current_idx);
    } else if (device == D2) {
        insert_end(&D2_queue_start, &D2_queue_end, current_idx);
    }

    if (ready_process == -1)
    {
        printf("Kernel: o filho %d acabou de fazer uma syscall, mas era o único executando. Vou ter que "
               "deixar a cpu parada\n",
               state->pid);
        switch_context(&process_states[current_idx], state, NULL);
    }
    else
    {
        switch_context(&process_states[current_idx], state, &process_states[ready_process]);
    }
}

void handle_irq0_timeslice(State* state, State process_states[])
{
    if (state->pid == -1) // cpu is idle
    {
        printf("Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.\n");
    }
    else
    {
        int current_idx = find_idx_from_pid(state->pid, process_states);
        int ready_process = pop_start(&ready_queue_start, &ready_queue_end);
        if (ready_process == -1)
        {
            printf("Kernel: o filho com pid %d é o único executando, vou deixar continuar mesmo tendo "
                    "acabado a fatia de tempo\n",
                    state->pid);
        }
        else
        {
            process_states[current_idx].current = READY;
            process_states[current_idx].is_running = 0;
            insert_end(&ready_queue_start, &ready_queue_end, current_idx);
            switch_context(&process_states[current_idx], state, &process_states[ready_process]);
        }
    }
}

void handle_irq1_device(State* state, State process_states[])
{
    int io_free_process = pop_start(&D1_queue_start, &D1_queue_end);
    if (io_free_process == -1) {
        printf("Kernel: recebi IRQ1, mas ninguém estava esperando. Nada acontece.\n");
    }
    else {
        if (state->pid == -1) {
            printf("Kernel: recebi IRQ1 sem ninguém rodando, vou liberar filho com pid %d\n", process_states[io_free_process].pid);
            switch_context(NULL, state, &process_states[io_free_process]);
        } else {
            int current_idx = find_idx_from_pid(state->pid, process_states);
            process_states[current_idx].current = READY;
            process_states[current_idx].is_running = 0;
            printf("Kernel: recebi IRQ1 vou liberar filho com pid %d\n", process_states[io_free_process].pid);
            switch_context(&process_states[current_idx], state, &process_states[io_free_process]);
        }
    }
}

void handle_irq2_device(State* state, State process_states[])
{
    int io_free_process = pop_start(&D2_queue_start, &D2_queue_end);
    if (io_free_process == -1) {
        printf("Kernel: recebi IRQ2, mas ninguém estava esperando. Nada acontece.\n");
    }
    else {
        if (state->pid == -1) {
            printf("Kernel: recebi IRQ2 sem ninguém rodando, vou liberar filho com pid %d\n", process_states[io_free_process].pid);
            switch_context(NULL, state, &process_states[io_free_process]);
        } else {
            int current_idx = find_idx_from_pid(state->pid, process_states);
            process_states[current_idx].current = READY;
            process_states[current_idx].is_running = 0;
            printf("Kernel: recebi IRQ2 vou liberar filho com pid %d\n", process_states[io_free_process].pid);
            switch_context(&process_states[current_idx], state, &process_states[io_free_process]);
        }
    }
}

void handle_irq(State* state, State process_states[], int irq_fifo_fd)
{
    enum irq_type buffer;
    read(irq_fifo_fd, &buffer, sizeof(enum irq_type));

    if (buffer == IRQ0) // time slice interrupt
    {
        handle_irq0_timeslice(state, process_states);
    }
    else if (buffer == IRQ1) // device 1 I/O interrupt
    {
        handle_irq1_device(state, process_states);
    }
    else if (buffer == IRQ2) // device 2 I/O interrupt
    {
        handle_irq2_device(state, process_states);
    }
    else
    {
        printf("Kernel: interrupção inválida\n");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    initialize_fifos();

    start_intercontroller();

    int irq_fifo_fd = open_irq_fifo();

    State *state = initialize_shared_memory();

    State process_states[NUM_APP_PROCESSES];
    initialize_process_states_array(process_states);

    create_application_processes(process_states);

    load_and_start_first_process(state, process_states);

    int syscall_fifo_fd = open_syscall_fifo();

    // preparing some select args
    int max_fd;
    struct timespec timeout;
    setup_pselect(irq_fifo_fd, syscall_fifo_fd, &max_fd, &timeout);

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
            if (state->pid != -1)
            {
                // stops running
                kill(state->pid, SIGSTOP);
                printf("Kernel: parei filho com pid %d\n", state->pid);
            }

            int syscall_pending = FD_ISSET(syscall_fifo_fd, &readfds);
            int irq_pending = FD_ISSET(irq_fifo_fd, &readfds);

            if (syscall_pending) // someone made a syscall
            {
                handle_syscall(state, process_states, syscall_fifo_fd);
            }

            if (irq_pending) // some irq came
            {
                handle_irq(state, process_states, irq_fifo_fd);
            }

            if (state->pid != -1) {
                // if not idle, start running
                state->current = RUNNING;
                state->is_running = 1;
                kill(state->pid, SIGCONT);
                printf("Kernel: continuei filho com pid %d\n", state->pid);
            }
        }

        else
        {
            printf("Nada aconteceu em 5 segundos, deve ter dado problema.\n");
        }
    }
}