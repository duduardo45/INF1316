#include "constants.h"
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    // conectar fifo com kernel
    int irq_fifo = open(IRQ_FIFO_PATH, O_WRONLY);
    if (irq_fifo < 0)
    {
        perror("erro ao abrir fifo");
    }

    enum irq_type buffer;

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = TIME_SLICE;

    while (1)
    {
        nanosleep(&tim, &tim2);
        buffer = IRQ0;
        write(irq_fifo, &buffer, sizeof(enum irq_type));
    }
}