#include "state.h"
#include <stdlib.h>

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
    case IDLE:
        state_str = "IDLE";
        break;
    default:
        state_str = "UNKNOWN";
        break;
    }
    printf("State {\n\t\
        pid: %d\n\t\
        PC: %d\n\t\
        current state: %s\n\t\
        syscall_args: is_shared=%d, offset=%d, path=%s, op=%c, payload=%s\n\t\
        is_running: %s\n\t\
        qt_syscalls: %d\n\t\
        done: %s\n\
        }\n",
           state->pid, state->PC, state_str, state->current_syscall.is_shared, state->current_syscall.offset,
           state->current_syscall.path, state->current_syscall.Op, state->current_syscall.payload,
           state->is_running ? "true" : "false", state->qt_syscalls, state->done ? "true" : "false");
}

void print_queue(Queue *start, char *name, State process_states[])
{
    printf("---------- %s start\t----------\n", name);
    while (start != NULL)
    {
        print_state(&process_states[start->process_pos]);
        start = start->next;
    }
    printf("---------- %s end\t----------\n", name);
}

void print_response_queue(ResponseQueue *start, char *name)
{
    printf("---------- %s start\t----------\n", name);
    while (start != NULL)
    {
        SfssResponse *resp = start->response_ptr;
        printf("Response {\n\t"
               "owner: %d\n\t"
               "ret_code: %d\n\t"
               "offset: %d\n\t"
               "payload: %s\n\t"
               "path: %s\n\t"
               "allfilenames: %s\n\t"
               "nrnames: %d\n\t"
               "}\n",
               resp->process_pos + 1, resp->response.ret_code, resp->response.offset, resp->response.payload,
               resp->response.path, resp->response.allfilenames, resp->response.nrnames);
        start = start->next;
    }
    printf("---------- %s end\t----------\n", name);
}

void insert_end(Queue **start, Queue **end, int process_pos)
{
    Queue *newNode = (Queue *)malloc(sizeof(Queue));
    if (newNode == NULL)
    {
        perror("Kernel: Nao consegui alocar memoria para o no da fila");
        exit(1);
    }
    newNode->process_pos = process_pos;
    newNode->next = NULL;
    if (*end == NULL)
    {
        *start = newNode;
        *end = newNode;
    }
    else
    {
        (*end)->next = newNode;
        *end = newNode;
    }
    return;
}
int pop_start(Queue **start, Queue **end)
{
    if (*start == NULL)
    {
        return -1;
    }

    Queue *temp = *start;
    int val = temp->process_pos;
    *start = temp->next;

    if (*start == NULL)
    {
        *end = NULL;
    }
    free(temp);
    return val;
}

void insert_end_response(ResponseQueue **start, ResponseQueue **end, SfssResponse *response_ptr)
{
    ResponseQueue *newNode = (ResponseQueue *)malloc(sizeof(ResponseQueue));
    if (newNode == NULL)
    {
        perror("Kernel: Nao consegui alocar memoria para o no da fila");
        exit(1);
    }
    newNode->response_ptr = response_ptr;
    newNode->next = NULL;
    if (*end == NULL)
    {
        *start = newNode;
        *end = newNode;
    }
    else
    {
        (*end)->next = newNode;
        *end = newNode;
    }
    return;
}
SfssResponse *pop_start_response(ResponseQueue **start, ResponseQueue **end)
{
    if (*start == NULL)
    {
        return NULL;
    }

    ResponseQueue *temp = *start;
    SfssResponse *response = temp->response_ptr;
    *start = temp->next;
    if (*start == NULL)
    {
        *end = NULL;
    }
    free(temp); // TODO: kernelSim has to free the response_ptr
    return response;
}