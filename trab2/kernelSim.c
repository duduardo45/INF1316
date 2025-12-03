/*

pseudo código:

first {
    define {
        D1_queue (IRQ_queue)
        D2_queue (IRQ_queue)
        ready_queue
        time_slice
        running
    }
    open core_state_sim (shared memory)
    create A1 : A5
    stop A1 : A5
    init A1 : A5 core_state

    move A2 : A5 to ready_queue
    move A1 to running
    start A1
}

when (IRQ) {
    check IRQ_queue
    stop running
    save core_state_sim
    if (P was waiting) {
        move P to top of ready_queue
    }
    move top of ready_queue to running
    load running core_state
    start running
}

when (syscall) {
    stop running
    save core_state_sim
    move running to IRQ_queue
    move top of ready_queue to running
    load running core_state
    start running
}

when (manual pause) {
    if (not paused) {
        stop running
        set paused
        dump running core_state
        print time_slice_remaining
        list ready_queue
        list D1_queue
        list D2_queue
    }
}

when (manual unpause) {
    if (paused) {
        unset paused
        start running
    }
}

while (not paused) {

}

*/

#include "constants.h"
#include "state.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


#define NUM_APP_PROCESSES 5

int paused = 0;

State *state;
State process_states[NUM_APP_PROCESSES];
pid_t inter_pid;

int num_children_done = 0;

int udp_sockfd;
struct sockaddr_in sfss_addr;

Queue *ready_queue_start = NULL;
Queue *ready_queue_end = NULL;
Queue *IRQ1_queue_start = NULL;
Queue *IRQ1_queue_end = NULL;
Queue *IRQ2_queue_start = NULL;
Queue *IRQ2_queue_end = NULL;

ResponseQueue *file_response_queue_start = NULL;
ResponseQueue *file_response_queue_end = NULL;
ResponseQueue *dir_response_queue_start = NULL;
ResponseQueue *dir_response_queue_end = NULL;

void manual_unpause(int num);
void manual_pause(int num)
{
    kill(inter_pid, SIGSTOP);
    paused = 1;
    (void)num;

    printf("\n\nPausa manual:\nVamos inspecionar o estado das coisas:\n\n");

    printf("Estado atual da cpu:\n");
    print_state(state);

    printf("\n");

    print_queue(ready_queue_start, "Ready Queue", process_states);

    print_queue(IRQ1_queue_start, "IRQ 1 IO Queue", process_states);
    print_queue(IRQ2_queue_start, "IRQ 2 IO Queue", process_states);

    print_response_queue(file_response_queue_start, "File Response Queue");
    print_response_queue(dir_response_queue_start, "Dir Response Queue");

    if (num_children_done)
    {
        printf("---------- All Processes\t----------\n");
        for (int i = 0; i < NUM_APP_PROCESSES; i++)
        {
            print_state(&process_states[i]);
        }
        printf("---------- All Processes end\t----------\n");
    }

    if (signal(SIGTSTP, manual_unpause) == SIG_ERR)
    {
        printf("Erro ao configurar despausa manual\n");
        exit(EXIT_FAILURE);
    }
}

void manual_unpause(int num)
{
    kill(inter_pid, SIGCONT);
    paused = 0;
    (void)num;

    if (signal(SIGTSTP, manual_pause) == SIG_ERR)
    {
        printf("Erro ao configurar pausa manual\n");
        exit(EXIT_FAILURE);
    }
}

void create_read_fifo(char path[])
{
    int fifo_done = mkfifo(path, 0666);

    if (fifo_done < 0)
    {
        if (errno != EEXIST)
        {
            perror("Não consegui criar fifo.");
            exit(EXIT_FAILURE);
        }
    }
}

/**
if cpu_state_pointer->pid == -1, then we are transitioning FROM idle CPU (should just start the new process)

if dest_state is NULL, then we are transitioning TO idle CPU (should just stop the current process)

otherwise, then we are context switching between two processes (should stop the current one and start the new
one)
*/
void switch_context(State *prev_state, State *cpu_state_pointer, State *dest_state)
{
    if (cpu_state_pointer->pid == -1 && dest_state == NULL)
    {
        printf("Argumentos de switch_context não fazem sentido\n");
        exit(EXIT_FAILURE);
    }

    if (cpu_state_pointer->pid != -1) // we are NOT transitioning FROM idle cpu
    {
        cpu_state_pointer->is_running = 0;
        if (prev_state != NULL)
            *prev_state = *cpu_state_pointer; // save cpu state so we can resume this process in the future
    }

    if (dest_state != NULL) // we are NOT transitioning TO idle cpu
    {
        *cpu_state_pointer = *dest_state; // change cpu state to hold the new process
    }
    else // we ARE transitioning TO idle CPU
    {
        cpu_state_pointer->pid = -1;
        cpu_state_pointer->PC = -1;
        cpu_state_pointer->current = IDLE;
        cpu_state_pointer->current_syscall.is_shared = 0;
        cpu_state_pointer->current_syscall.offset = 0;
        strcpy(cpu_state_pointer->current_syscall.path, "");
        cpu_state_pointer->current_syscall.Op = NO_OPERATION;
        cpu_state_pointer->is_running = 0;
        cpu_state_pointer->qt_syscalls = -1;
        cpu_state_pointer->done = 1;
    }
}

/**
returns the index of the process with a specific pid in process_states, fails fast if not found
 */
int find_idx_from_pid(pid_t pid, State process_states[])
{
    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        if (process_states[i].pid == pid)
        {
            return i;
        }
    }

    exit(EXIT_FAILURE);
    return -1;
}

/** Returns NULL if response not found in the queue */
SfssResponse *find_response_from_process(int process_pos)
{
    SfssResponse *response = pop_start_response(&file_response_queue_start, &file_response_queue_end);
    if (response == NULL)
    {
        return NULL;
    }
    // if the first response wasn't yours, im gonna search the next ones, just a sec
    int tries = NUM_APP_PROCESSES;
    while (response->process_pos != process_pos && tries > 0)
    {
        insert_end_response(&file_response_queue_start, &file_response_queue_end, response);
        response = pop_start_response(&file_response_queue_start, &file_response_queue_end);
        tries--;
    }
    if (tries == 0)
    {
        return NULL;
    }
    // now we have the response for the process
    return response;
}

// UDP functions

// Inicializa a rede e faz bind na porta do Kernel
void initialize_network(void) {
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        perror("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in kernel_addr;
    memset(&kernel_addr, 0, sizeof(kernel_addr));
    kernel_addr.sin_family = AF_INET;
    kernel_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    kernel_addr.sin_port = htons(KERNEL_PORT);

    if (bind(udp_sockfd, (struct sockaddr *)&kernel_addr, sizeof(kernel_addr)) < 0) {
        perror("Erro no bind do Kernel");
        exit(EXIT_FAILURE);
    }

    memset(&sfss_addr, 0, sizeof(sfss_addr));
    sfss_addr.sin_family = AF_INET;
    sfss_addr.sin_port = htons(SFSS_PORT);
    inet_aton(SFSS_IP, &sfss_addr.sin_addr);
}

// Envia SfssRequest via UDP
void send_request_to_sfss(int process_pos, syscall_args args) {
    SfssRequest req;
    req.process_pos = process_pos;
    req.args = args;

    if (sendto(udp_sockfd, &req, sizeof(req), 0, (struct sockaddr *)&sfss_addr, sizeof(sfss_addr)) < 0) {
        perror("Kernel: Erro ao enviar UDP para SFSS");
    } else {
        printf("Kernel: Enviado UDP REQ (Owner: %d, Op: %c, Path: %s)\n", process_pos, args.Op, args.path);
    }
}

// Recebe SfssResponse e enfileira
void handle_udp_response() {
    SfssResponse buffer_resp;
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    if (recvfrom(udp_sockfd, &buffer_resp, sizeof(buffer_resp), 0, (struct sockaddr *)&sender_addr, &sender_len) > 0) {
        printf("Kernel: Recebi UDP REP (Owner: %d, Ret: %d)\n", buffer_resp.process_pos, buffer_resp.response.ret_code);

        // Aloca resposta para colocar na fila
        SfssResponse *new_node = (SfssResponse *)malloc(sizeof(SfssResponse));
        *new_node = buffer_resp;

        // Decide em qual fila colocar baseado no tipo de operação original ou resposta
        // BACALHAU TODO: Tirar hardcode da fila de resposta de arquivo, tem que ser dinâmico arquivo/diretório
        insert_end_response(&file_response_queue_start, &file_response_queue_end, new_node);
    }
}


void initialize_fifos(void)
{
    create_read_fifo(IRQ_FIFO_PATH);
    create_read_fifo(SYSCALL_FIFO_PATH);
}

void start_intercontroller(void)
{
    // start interController process
    inter_pid = fork();

    if (inter_pid == 0)
    {
        char *argv[] = {"interControllerSim", NULL};
        execv("./build/interControllerSim", argv);
        perror("Não consegui dar execv.");
        exit(EXIT_FAILURE);
    }
}

int open_irq_fifo(void)
{
    int irq_fifo_fd = open(IRQ_FIFO_PATH, O_RDONLY);

    if (irq_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }
    return irq_fifo_fd;
}

State *initialize_shared_memory(void)
{
    // init core state shmem
    int shmid = shmget(CORE_STATE_SHMEM_KEY, sizeof(State),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0)
    {
        perror("Não consegui pegar shmem");
        exit(EXIT_FAILURE);
    }

    State *state = (State *)shmat(shmid, 0, 0);
    return state;
}

void initialize_process_states_array(State process_states[])
{
    // initializing core states of all processes
    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        process_states[i].pid = -1; // -1 for now, will be set after forks
        process_states[i].PC = 0;
        process_states[i].current = READY;
        syscall_args current_syscall = {.is_shared = 0, .offset = 0, .path = "", .Op = NO_OPERATION, .payload = ""};
        process_states[i].current_syscall = current_syscall;
        syscall_response current_response = {.ret_code = EMPTY, .offset = 0, .len = 0, .payload = ""};
        process_states[i].current_response = current_response;
        process_states[i].is_running = 0;
        process_states[i].qt_syscalls = 0;
        process_states[i].done = 0;
    }
}

void create_application_processes(State process_states[])
{
    // creating application processes
    for (int i = 0; i < NUM_APP_PROCESSES; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork deu errado");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        { // child
            char *argv[] = {"A", NULL};
            execv("./build/A", argv);
            perror("Não dei exec!");
            exit(EXIT_FAILURE);
        }
        else
        { // parent
            kill(pid, SIGSTOP);
        }

        process_states[i].pid = pid;
        printf("Kernel: iniciei filho %d com pid %d\n", i, pid);
    }
}

void load_and_start_first_process(State *state, State process_states[])
{
    // load first process
    *state = process_states[0];
    (*state).current = RUNNING;
    (*state).is_running = 1;

    State *temp;
    // fill the ready queue
    for (int i = 1; i < NUM_APP_PROCESSES; i++)
    {
        temp = &process_states[i];
        (*temp).current = READY;
        (*temp).is_running = 0;
        insert_end(&ready_queue_start, &ready_queue_end, i);
    }

    kill(state->pid, SIGCONT);
    printf("Kernel: comecei com filho de pid %d\n", state->pid);
}

int open_syscall_fifo(void)
{
    int syscall_fifo_fd = open(SYSCALL_FIFO_PATH, O_RDONLY | O_NONBLOCK);

    if (syscall_fifo_fd < 0)
    {
        perror("erro ao abrir fifo");
    }
    return syscall_fifo_fd;
}

void setup_pselect(int irq_fifo_fd, int syscall_fifo_fd, int *max_fd)
{
    // preparing some select args
    int temp_max = (irq_fifo_fd >= syscall_fifo_fd) ? irq_fifo_fd : syscall_fifo_fd;
    *max_fd = (temp_max >= udp_sockfd) ? temp_max : udp_sockfd;
}

void clear_syscall_args(State *state)
{
    state->current_syscall.is_shared = 0;
    state->current_syscall.offset = 0;
    strcpy(state->current_syscall.path, "");
    state->current_syscall.Op = NO_OPERATION;
    strcpy(state->current_syscall.payload, "");
}

void handle_syscall(State *state, State process_states[], int syscall_fifo_fd)
{
    if (state->pid == -1) // cpu is idle
    {
        printf("Kernel: situação impossível: cpu está idle e acabei de receber uma syscall!\n");
        exit(EXIT_FAILURE);
    }

    int current_idx = find_idx_from_pid(state->pid, process_states);
    int ready_process = pop_start(&ready_queue_start, &ready_queue_end);

    syscall_args args;
    // Processes the syscall
    if (read(syscall_fifo_fd, &args, sizeof(args)) == -1)
    {
        // Unexpected FIFO reading error
        perror("Kernel: erro ao ler syscall_fifo_fd");
        exit(EXIT_FAILURE);
    }

    // syscall handling logic
    state->current_syscall = args;
    state->current = WAITING_FOR_IO;
    state->qt_syscalls++;

    printf(
        "Kernel: processo anterior fez syscall, com args: is_shared=%d, offset=%d, path='%s', op='%c', payload='%s'\n",
        args.is_shared, args.offset, args.path, args.Op, args.payload);

    // TODO: should include DR as a file or folder operation?
    if (args.Op == RD || args.Op == WR) // file operation
    {
        insert_end(&IRQ1_queue_start, &IRQ1_queue_end, current_idx);
        send_request_to_sfss(current_idx, args);
    }
    else if (args.Op == DL || args.Op == DC) // directory operation
    {
        insert_end(&IRQ2_queue_start, &IRQ2_queue_end, current_idx);
    }
    else if (args.Op == NO_OPERATION && strcmp(args.path, "") == 0) // convention for exit syscall
    {
        state->current = DONE;
        state->done = 1;
        num_children_done++;
        if (num_children_done == NUM_APP_PROCESSES)
        {
            printf("Kernel: todos meus filhos executaram até o fim... Sou um kernel feliz!\n");
            exit(0);
        }
    }

    if (ready_process == -1)
    {
        printf("Kernel: o filho %d acabou de fazer uma syscall, mas era o único executando. Vou ter que "
               "deixar a cpu parada\n",
               state->pid);
        switch_context(&process_states[current_idx], state, NULL);
    }
    else
    {
        switch_context(&process_states[current_idx], state, &process_states[ready_process]);
    }
}

void handle_irq0_timeslice(State *state, State process_states[])
{
    if (state->pid == -1) // cpu is idle
    {
        printf("Kernel: recebi IRQ0, mas cpu está idle. Nada acontece.\n");
    }
    else
    {
        int current_idx = find_idx_from_pid(state->pid, process_states);
        int ready_process = pop_start(&ready_queue_start, &ready_queue_end);
        if (ready_process == -1)
        {
            printf("Kernel: o filho com pid %d é o único executando, vou deixar continuar mesmo tendo "
                   "acabado a fatia de tempo\n",
                   state->pid);
        }
        else
        {
            state->current = READY;
            state->is_running = 0;
            insert_end(&ready_queue_start, &ready_queue_end, current_idx);
            switch_context(&process_states[current_idx], state, &process_states[ready_process]);
        }
    }
}

void handle_irq1_device(State *state, State process_states[])
{
    int io_free_process = pop_start(&IRQ1_queue_start, &IRQ1_queue_end);
    if (io_free_process == -1)
    {
        printf("Kernel: recebi IRQ1, mas ninguém estava esperando. Nada acontece.\n");
    }
    else
    {
        if (state->pid == -1)
        {
            printf("Kernel: recebi IRQ1 sem ninguém rodando. Vou procurar a resposta para o filho com pid %d\n",
                   process_states[io_free_process].pid);

            SfssResponse *response = find_response_from_process(io_free_process);
            if (response == NULL)
            {
                printf("Kernel: recebi IRQ1 para o filho com pid %d, mas não encontrei a resposta. Vou colocar ele no "
                       "final da fila\n",
                       process_states[io_free_process].pid);
                insert_end(&IRQ1_queue_start, &IRQ1_queue_end, io_free_process);
                return;
            }
            printf("Kernel: encontrei a resposta para o filho com pid %d. Agora vou liberá-lo\n",
                   process_states[io_free_process].pid);

            process_states[io_free_process].current_response = response->response;

            // clearing syscall args since we have handled the syscall
            clear_syscall_args(&process_states[io_free_process]);

            switch_context(NULL, state, &process_states[io_free_process]);
        }
        else
        {
            int current_idx = find_idx_from_pid(state->pid, process_states);
            state->current = READY;
            state->is_running = 0;

            printf("Kernel: recebi IRQ1. Vou procurar a resposta para o filho com pid %d\n",
                   process_states[io_free_process].pid);

            SfssResponse *response = find_response_from_process(io_free_process);
            if (response == NULL)
            {
                printf("Kernel: recebi IRQ1 para o filho com pid %d, mas não encontrei a resposta. Vou colocar ele no "
                       "final da fila\n",
                       process_states[io_free_process].pid);
                insert_end(&IRQ1_queue_start, &IRQ1_queue_end, io_free_process);
                return;
            }
            printf("Kernel: encontrei a resposta para o filho com pid %d. Agora vou liberá-lo\n",
                   process_states[io_free_process].pid);

            process_states[io_free_process].current_response = response->response;

            // clearing syscall args since we have handled the syscall
            clear_syscall_args(&process_states[io_free_process]);

            switch_context(&process_states[current_idx], state, &process_states[io_free_process]);

            // current process goes to the end of the ready queue, it got preempted
            insert_end(&ready_queue_start, &ready_queue_end, current_idx);
        }
    }
}

void handle_irq2_device(State *state, State process_states[])
{
    int io_free_process = pop_start(&IRQ2_queue_start, &IRQ2_queue_end);
    if (io_free_process == -1)
    {
        printf("Kernel: recebi IRQ2, mas ninguém estava esperando. Nada acontece.\n");
    }
    else
    {
        if (state->pid == -1)
        {
            printf("Kernel: recebi IRQ2 sem ninguém rodando. Vou procurar a resposta para o filho com pid %d\n",
                   process_states[io_free_process].pid);

            SfssResponse *response = find_response_from_process(io_free_process);
            if (response == NULL)
            {
                printf("Kernel: recebi IRQ2 para o filho com pid %d, mas não encontrei a resposta. Vou colocar ele no "
                       "final da fila\n",
                       process_states[io_free_process].pid);
                insert_end(&IRQ2_queue_start, &IRQ2_queue_end, io_free_process);
                return;
            }
            printf("Kernel: encontrei a resposta para o filho com pid %d. Agora vou liberá-lo\n",
                   process_states[io_free_process].pid);

            process_states[io_free_process].current_response = response->response;

            // clearing syscall args since we have handled the syscall
            clear_syscall_args(&process_states[io_free_process]);

            switch_context(NULL, state, &process_states[io_free_process]);
        }
        else
        {
            int current_idx = find_idx_from_pid(state->pid, process_states);
            state->current = READY;
            state->is_running = 0;

            printf("Kernel: recebi IRQ2. Vou procurar a resposta para o filho com pid %d\n",
                   process_states[io_free_process].pid);

            SfssResponse *response = find_response_from_process(io_free_process);
            if (response == NULL)
            {
                printf("Kernel: recebi IRQ2 para o filho com pid %d, mas não encontrei a resposta. Vou colocar ele no "
                       "final da fila\n",
                       process_states[io_free_process].pid);
                insert_end(&IRQ2_queue_start, &IRQ2_queue_end, io_free_process);
                return;
            }
            printf("Kernel: encontrei a resposta para o filho com pid %d. Agora vou liberá-lo\n",
                   process_states[io_free_process].pid);

            process_states[io_free_process].current_response = response->response;

            // clearing syscall args since we have handled the syscall
            clear_syscall_args(&process_states[io_free_process]);

            switch_context(&process_states[current_idx], state, &process_states[io_free_process]);

            // current process goes to the end of the ready queue, it got preempted
            insert_end(&ready_queue_start, &ready_queue_end, current_idx);
        }
    }
}

void handle_irq(State *state, State process_states[], int irq_fifo_fd)
{
    enum irq_type buffer;
    read(irq_fifo_fd, &buffer, sizeof(enum irq_type));

    if (buffer == IRQ0) // time slice interrupt
    {
        handle_irq0_timeslice(state, process_states);
    }
    else if (buffer == IRQ1) // device 1 I/O interrupt
    {
        handle_irq1_device(state, process_states);
    }
    else if (buffer == IRQ2) // device 2 I/O interrupt
    {
        handle_irq2_device(state, process_states);
    }
    else
    {
        printf("Kernel: interrupção inválida\n");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    initialize_fifos();
    initialize_network();

    // setup manual pause
    if (signal(SIGTSTP, manual_pause) == SIG_ERR)
    {
        printf("Erro ao configurar pausa manual\n");
        exit(EXIT_FAILURE);
    }

    start_intercontroller();

    int irq_fifo_fd = open_irq_fifo();

    state = initialize_shared_memory();

    initialize_process_states_array(process_states);

    create_application_processes(process_states);

    load_and_start_first_process(state, process_states);

    int syscall_fifo_fd = open_syscall_fifo();

    // preparing some select args
    int max_fd;

    setup_pselect(irq_fifo_fd, syscall_fifo_fd, &max_fd);

    // normal operation starts
    while (1)
    {
        if (paused)
        {
            sleep(1);
            continue;
        }
        // using select in order to "keep up with" both irq and syscall file descriptors at the same time
        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(irq_fifo_fd, &readfds);
        FD_SET(syscall_fifo_fd, &readfds);
        FD_SET(udp_sockfd, &readfds);

        struct timespec timeout;
        timeout.tv_sec = 5;
        timeout.tv_nsec = 0;
        int num_ready = pselect(max_fd + 1, &readfds, NULL, NULL, &timeout, NULL);

        if (num_ready == -1)
        {
            if (errno == EINTR)
            {
                continue; // handle the manual pause expected error
            }
            perror("select()");
        }
        else if (num_ready) // either irq or syscall came
        {
            if (state->pid != -1)
            {
                // stops running
                kill(state->pid, SIGSTOP);
                printf("Kernel: parei filho com pid %d\n", state->pid);
            }

            int syscall_pending = FD_ISSET(syscall_fifo_fd, &readfds);
            int irq_pending = FD_ISSET(irq_fifo_fd, &readfds);
            int udp_pending = FD_ISSET(udp_sockfd, &readfds);

            if (udp_pending) {
                handle_udp_response(); // Processa chegada de pacotes da rede
            }

            if (syscall_pending) // someone made a syscall
            {
                handle_syscall(state, process_states, syscall_fifo_fd);
            }
            // sometimes it can happen in the log that:
            /*
            Kernel: recebi IRQ2 sem ninguém rodando, vou liberar filho com pid 16972
            Kernel: continuei filho com pid 16972
            Processo 16972: fiz syscall, com args: device=2 e op=X
            Kernel: parei filho com pid 16972
            Kernel: o filho com pid 16972 é o único executando, vou deixar continuar mesmo tendo acabado a fatia de
            tempo
            Kernel: continuei filho com pid 16972
            Processo 16972: acabei iteração 28
            */
            // this is still correct because the 16972 process was interrupted after sending the syscall but
            // before reporting to the log that it did, so when it was unpaused because the IO was done,
            // it continued from where it stopped which was right before printing that it did a syscall.
            if (irq_pending) // some irq came
            {
                handle_irq(state, process_states, irq_fifo_fd);
            }

            if (state->pid != -1)
            {
                // if not idle, start running
                state->current = RUNNING;
                state->is_running = 1;
                kill(state->pid, SIGCONT);
                printf("Kernel: continuei filho com pid %d\n", state->pid);
            }
        }

        else
        {
            printf("Nada aconteceu em 5 segundos, deve ter dado problema.\n");
        }
    }
}