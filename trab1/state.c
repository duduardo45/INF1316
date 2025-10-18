#include "state.h"

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
        syscall_args: device=%d e op=%c\n\t\
        is_running: %s\n\t\
        qt_syscalls: %d\n\t\
        done: %s\n\
        }\n",
           state->PC, state_str, state->current_syscall.Dx, state->current_syscall.Op,
           state->is_running ? "true" : "false", state->qt_syscalls, state->done ? "true" : "false");
}