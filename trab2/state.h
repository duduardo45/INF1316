#pragma once

#include "constants.h"
#include <stdio.h>
#include <sys/types.h>

typedef struct syscall_args
{
    // all operation types
    int owner; // this can be 0, in case process is using A0 subdirectory, and so cannot be inferred from current
               // running process in KernelSim
    int offset;
    char *path;

    enum operation_type Op; // operation type (RD, WR, DC, DL, DR)
    // RD and WR
    char payload[16];
    // // DC and DR and DL
    // int len1;
    // // DC and DR
    // int len2;
    // char *name;
    // // DL
    // char *all_dir_info;
    // int fstlstpositions[40];
    // int *nrnames;

} syscall_args;

typedef struct state_t
{
    pid_t pid;                  // process id. if -1, CPU is idle
    int PC;                     // program counter
    enum current_state current; // current state
    syscall_args
        current_syscall; // current syscall being processed. check if current == WAITING_FOR_IO to see if this is valid
    int is_running;      // whether the process is currently running
    int qt_syscalls;     // quantity of syscalls made
    int done;            // whether the process has finished execution
} State;

typedef struct queue_t
{
    struct queue_t *next;
    int process_pos;
} Queue;

void print_state(State *state);
void print_queue(Queue *start, char *name, State process_states[]);
void insert_end(Queue **start, Queue **end, int process_pos);
int pop_start(Queue **start, Queue **end);