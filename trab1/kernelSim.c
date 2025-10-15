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
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_APP_PROCESSES 5

int main(void)
{
    // connect fifo with intercontroller
    int fifo_done = mkfifo("/tmp/irq_fifo", 0666);

    if (fifo_done < 0)
    {
        if (errno != EEXIST)
        {
            perror("Não consegui criar fifo.");
            exit(EXIT_FAILURE);
        }
    }

    pid_t inter_pid = fork();
    if (inter_pid == 0)
    {
        char *argv[] = {"interControllerSim", NULL};
        execv("./build/interControllerSim", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }

    int irq_fifo = open("/tmp/irq_fifo", O_RDONLY);
    if (irq_fifo < 0)
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

    // State *state = (State *)shmat(shmid, 0, 0);

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

    // normal operation starts

    int running_child = 0;

    kill(pids[running_child], SIGCONT);

    while (1)
    {
        enum irq_type buffer;
        read(irq_fifo, &buffer, sizeof(enum irq_type));

        if (buffer == IRQ0) // time slice interrupt
        {
            kill(pids[running_child], SIGSTOP);
            printf("Kernel: parei filho %d com pid %d\n", running_child, pids[running_child]);

            running_child++;
            running_child %= NUM_APP_PROCESSES;

            kill(pids[running_child], SIGCONT);
            printf("Kernel: continuei filho %d com pid %d\n", running_child, pids[running_child]);
        }
        else // device I/O interrupt
        {
            // TODO: implement
        }
    }
}