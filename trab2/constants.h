#pragma once
#define SYSCALL_FIFO_PATH "/tmp/syscall_fifo"
#define IRQ_FIFO_PATH "/tmp/irq_fifo"
#define P_1 0.1
#define P_2 0.005

#define SFSS_IP "127.0.0.1"
#define SFSS_PORT 9876
#define KERNEL_PORT 9875 // Porta fixa para o Kernel receber respostas
#define SFSS_ROOT "./SFSS-root-dir" // Diretório raiz do servidor

#define MAX_FILENAME_LEN 25
#define MAX_PATH_LEN 100
#define MAX_DIR_ENTRIES 40

enum irq_type
{
    IRQ0, // time slice interrupt
    IRQ1, // Wakeup for SFSS file reply (if exists)
    IRQ2  // Wakeup for SFSS directory reply (if exists)
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
    EXIT = 'E',
    RD = 'R',
    WR = 'W',
    DC = 'C',
    DR = 'D', // Delete (R from Remove is already taken)
    DL = 'L'
};
