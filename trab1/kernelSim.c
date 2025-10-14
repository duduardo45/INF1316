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
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct core_state_sim
{
    int reg1;
    int reg2;
    int reg3;
    int reg4;
} CoreStateSim;

int main(void)
{
    int shmid = shmget(IPC_PRIVATE, sizeof(CoreStateSim), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
    }

    CoreStateSim *pcore_state = (CoreStateSim *)shmat(shmid, 0, 0);
}