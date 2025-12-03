#pragma once
#define SYSCALL_FIFO_PATH "/tmp/syscall_fifo"
#define IRQ_FIFO_PATH "/tmp/irq_fifo"
#define P_1 0.1
#define P_2 0.005

#define SFSS_IP "127.0.0.1"
#define SFSS_PORT 9876
#define KERNEL_PORT 9875 // Porta fixa para o Kernel receber respostas
#define SFSS_ROOT "./SFSS-root-dir" // Diretório raiz do servidor

enum irq_type
{
    IRQ0, // time slice interrupt
    IRQ1, // SFSS replied to file operation
    IRQ2  // SFSS replied to directory operation
};

enum
{
    CORE_STATE_SHMEM_KEY =
        7250,               // change this every time you change the State struct (or delete the old shmem in that key)
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

enum operation_type
{
    NO_OPERATION = 'N',
    RD = 'R',
    WR = 'W',
    DC = 'C',
    DR = 'D', // Delete (R from Remove is already taken)
    DL = 'L'
};
