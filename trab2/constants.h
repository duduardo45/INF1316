#pragma once
#define SYSCALL_FIFO_PATH "/tmp/syscall_fifo"
#define IRQ_FIFO_PATH "/tmp/irq_fifo"
#define P_1 0.1
#define P_2 0.005

enum irq_type
{
    IRQ0, // time slice interrupt
    IRQ1, // SFSS replied to file operation
    IRQ2  // SFSS replied to directory operation
};

enum
{
    CORE_STATE_SHMEM_KEY = 7249,
    TIME_SLICE = 500000000, // nanoseconds
    A_SLEEP = 500000000,    // nanoseconds
};

enum current_state
{
    RUNNING,
    READY,
    WAITING_FOR_IO,
    TERMINATED,
    DONE,
    IDLE
};

enum device_number
{
    NO_DEVICE = 'N',
    D1 = '1',
    D2 = '2'
};

enum operation_type
{
    NO_OPERATION = 'N',
    RD = 'R',
    WR = 'W',
    DC = 'C',
    DR = 'M', // reMove (D and R are already taken)
    DL = 'L'
};
