enum irq_type
{
    IRQ0, // time slice interrupt
    IRQ1, // device 1 ended I/O interrupt
    IRQ2  // device 2 ended I/O interrupt
};

enum
{
    CORE_STATE_SHMEM_KEY = 7249,
    TIME_SLICE = 500000000, // nanoseconds
    A_SLEEP = 500000000,    // nanoseconds
};
