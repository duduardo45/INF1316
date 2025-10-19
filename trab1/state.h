#pragma once

#include <stdio.h>
#include "constants.h"

typedef struct syscall_args
{
    enum device_number Dx;  // which device (D1 or D2)
    enum operation_type Op; // operation type (R, W, or X)
} syscall_args;

typedef struct state_t
{
    int pid;                    // process id. if -1, CPU is idle
    int PC;                     // program counter
    enum current_state current; // current state
    syscall_args
        current_syscall; // current syscall being processed. check if current == WAITING_FOR_IO to see if this is valid
    int is_running;      // whether the process is currently running
    int qt_syscalls;     // quantity of syscalls made
    int done;            // whether the process has finished execution
} State;

typedef struct queue_t {
    struct queue_t* next;
    int process_pos;
} Queue;

void print_state(State *state);
void insert_end(Queue** start, Queue** end, int process_pos);
int pop_start(Queue** start, Queue** end);