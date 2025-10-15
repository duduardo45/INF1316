#pragma once

#include <stdio.h>

enum current_state
{
    RUNNING,
    READY,
    WAITING_FOR_IO,
    TERMINATED,
    DONE
};

enum device_number
{
    D1 = 1,
    D2 = 2
};

enum operation_type
{
    R = 'R',
    W = 'W',
    X = 'X'
};

typedef struct syscall_args
{
    enum device_number Dx;  // which device (D1 or D2)
    enum operation_type Op; // operation type (R, W, or X)
} syscall_args;

typedef struct state_t
{
    int PC;                       // program counter
    enum current_state current;   // current state
    int blocked_by_device;        // which device is blocking the process (if any)
    syscall_args current_syscall; // current syscall being processed
    int is_running;               // whether the process is currently running
    int qt_syscalls;              // quantity of syscalls made
    int done;                     // whether the process has finished execution
} State;

void print_state(State *state)
{
    const char *state_str;
    switch (state->current)
    {
    case RUNNING:
        state_str = "RUNNING";
        break;
    case READY:
        state_str = "READY";
        break;
    case WAITING_FOR_IO:
        state_str = "WAITING_FOR_IO";
        break;
    case TERMINATED:
        state_str = "TERMINATED";
        break;
    case DONE:
        state_str = "DONE";
        break;
    default:
        state_str = "UNKNOWN";
        break;
    }
    printf("State {\n\t\
        PC: %d\n\t\
        current state: %s\n\t\
        blocked_by_device: %d\n\t\
        is_running: %s\n\t\
        qt_syscalls: %d\n\t\
        done: %s\n\
        }\n",
           state->PC,
           state_str,
           state->blocked_by_device,
           state->is_running ? "true" : "false",
           state->qt_syscalls,
           state->done ? "true" : "false");
}