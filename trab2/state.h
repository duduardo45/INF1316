#pragma once

#include "constants.h"
#include <stdio.h>
#include <sys/types.h>

typedef struct syscall_args
{
    // all operation types
    int is_shared; // whether the operation happens to the shared directory (A0)
    int offset;
    char path[MAX_PATH_LEN]; // should NOT contain leading or trailing slashes

    enum operation_type Op; // operation type (RD, WR, DC, DL, DR)
    // RD and DL
    // nothing
    // WR
    char payload[16];
    // DC and DR
    char dir_name[MAX_FILENAME_LEN]; // should NOT have leading or trailing slashes
} syscall_args;

enum ret_code
{
    ERROR = -1,
    EMPTY = 0,
    SUCCESS = 1
};

typedef struct syscall_response // BACALHAU
{
    enum ret_code ret_code; // return code of the syscall
    int offset;             // this should be negative for errors

    // WR
    // nothing

    // RD
    char payload[16];

    // DC and DR
    char path[MAX_PATH_LEN]; // should contain leading or trailing slashes

    // dados específicos do DC e DR

    // dados específicos do DL
    char allfilenames[MAX_FILENAME_LEN * MAX_DIR_ENTRIES]; // up to 40 files/directories listed, each with 25 chars
    struct
    {
        int start;
        int end;
        int type;
    } fstlstpositions[MAX_DIR_ENTRIES];
    int nrnames;
} syscall_response;

typedef struct sfss_request
{
    int process_pos; // Funciona como o "owner" no protocolo
    syscall_args args;
} SfssRequest;

typedef struct sfss_response
{
    int process_pos;
    syscall_response response;
} SfssResponse;

typedef struct state_t
{
    pid_t pid;                  // process id. if -1, CPU is idle
    int PC;                     // program counter
    enum current_state current; // current state
    syscall_args
        current_syscall; // current syscall being processed. check if current == WAITING_FOR_IO to see if this is valid
    syscall_response current_response; // current response to be processed.
    int is_running;                    // whether the process is currently running
    int qt_syscalls;                   // quantity of syscalls made
    int done;                          // whether the process has finished execution
} State;

typedef struct queue_t
{
    struct queue_t *next;
    int process_pos;
} Queue;

typedef struct response_queue_t
{
    struct response_queue_t *next;
    SfssResponse *response_ptr;
} ResponseQueue;

void print_state(State *state);
void print_queue(Queue *start, char *name, State process_states[]);
void print_response_queue(ResponseQueue *start, char *name);
void insert_end(Queue **start, Queue **end, int process_pos);
int pop_start(Queue **start, Queue **end);

void insert_end_response(ResponseQueue **start, ResponseQueue **end, SfssResponse *response_ptr);
SfssResponse *pop_start_response(ResponseQueue **start, ResponseQueue **end);