#include "constants.h"
#include "state.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX 10
#define SYSCALL_PROBABILITY 40
#define NUM_WORDS 5

const char *words[NUM_WORDS] = {"dados", "teste", "foto", "log", "doc"};

State *state;

// Gera um nome aleatório combinando duas palavras, ex: "dados_log"
void generate_random_name(char *buffer)
{
    int idx1 = rand() % NUM_WORDS;
    int idx2 = rand() % NUM_WORDS;

    // Garante que não pega a mesma palavra duas vezes (opcional, mas fica mais bonito)
    while (idx2 == idx1)
    {
        idx2 = rand() % NUM_WORDS;
    }

    sprintf(buffer, "%s_%s", words[idx1], words[idx2]);
}

void syscall_sim(pid_t mypid, int syscall_fifo, syscall_args args)
{
    printf("Processo %d: vou fazer syscall, com args: is_shared=%d, offset=%d, path='%s', op='%c', payload='%s'\n",
           mypid, args.is_shared, args.offset, args.path, args.Op, args.payload);
    write(syscall_fifo, &args, sizeof(args));
    printf("Processo %d: fiz syscall, com args: is_shared=%d, offset=%d, path='%s', op='%c', payload='%s'\n", mypid,
           args.is_shared, args.offset, args.path, args.Op, args.payload);
}

void generate_random_payload(char *buffer, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size - 1; i++)
    {
        int key = rand() % (sizeof(charset) - 1);
        buffer[i] = charset[key];
    }
    buffer[size - 1] = '\0';
}

int pick_existing_type(char *buffer, char type)
{
    // Tenta buscar diretamente do state, assumindo que os dados do último DL persistem
    if (state->current_response.nrnames > 0)
    {
        int start_idx = rand() % state->current_response.nrnames;
        // Percorre circularmente para tentar achar um arquivo
        for (int i = 0; i < state->current_response.nrnames; i++)
        {
            int idx = (start_idx + i) % state->current_response.nrnames;
            if (state->current_response.fstlstpositions[idx].type == type)
            {
                int start = state->current_response.fstlstpositions[idx].start;
                int end = state->current_response.fstlstpositions[idx].end;
                int len = end - start + 1;
                if (len < MAX_FILENAME_LEN)
                {
                    strncat(buffer, &state->current_response.allfilenames[start], len);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void pick_existing_file(char *buffer)
{
    if (!pick_existing_type(buffer, TYPE_FILE))
    {
        strcat(buffer, "mypath"); // Fallback
    }
}

void pick_existing_directory(char *buffer)
{
    pick_existing_type(buffer, TYPE_DIR);
    // Se não achar, string vazia (raiz)
}

/** returns 1 if syscall happened, 0 otherwise */
enum operation_type maybe_syscall(pid_t mypid, int syscall_fifo)
{
    int d = (rand() % 100);
    if (d < SYSCALL_PROBABILITY) // generate a random syscall with low probability
    {

        syscall_args args = {
            .is_shared = 0, .offset = 0, .path = "", .Op = NO_OPERATION, .payload = "", .dir_name = ""};

        enum operation_type op;

        int op_choice = 4; //(rand() % 5);
        int offset_val;
        char temp_buffer[100];

        switch (op_choice)
        {
        case 0: // WR
            op = WR;
            offset_val = (rand() % 7) * 16;
            args.offset = offset_val;
            generate_random_payload(args.payload, sizeof(args.payload));

            // Escolha entre criar um novo arquivo ou um que já existe
            if (rand() % 5 == 0)
            {
                if (rand() % 2 == 0)
                {
                    char buffer[100] = "";
                    pick_existing_directory(buffer);
                    strcat(args.path, buffer);
                    strcat(args.path, "/");
                    generate_random_name(temp_buffer);
                    strcat(args.path, temp_buffer);
                }
                else
                    generate_random_name(args.path);
            }
            else
            {
                pick_existing_file(args.path);
            }
            break;
        case 1: // RD
            op = RD;
            offset_val = (rand() % 7) * 16;
            args.offset = offset_val;
            pick_existing_file(args.path);
            break;
        case 2: // DC
            op = DC;
            generate_random_name(args.dir_name);
            if (rand() % 5 == 0)
                pick_existing_directory(args.path);
            break;
        case 3: // DR
            op = DR;
            if (rand() % 5 == 0)
                pick_existing_directory(args.path);
            else
                pick_existing_file(args.path);
            break;
        case 4: // DL
            op = DL;
            if (rand() % 5 == 0)
                pick_existing_directory(args.path);
            break;
        default:
            printf("Resto inválido\n");
            exit(EXIT_FAILURE);
            break;
        }

        args.Op = op;

        args.is_shared = ((rand() % 5) == 0); // 20% de chance de ser na pasta compartilhada

        syscall_sim(mypid, syscall_fifo, args);
        return op;
    }
    return NO_OPERATION;
}

void print_response(pid_t mypid, State *state, enum operation_type last_syscall_op)
{
    printf("Processo %d: recebi resposta: ret_code=%d. ", mypid, state->current_response.ret_code);

    if (state->current_response.ret_code == ERROR)
    {
        printf("A operação falhou.\n");
    }
    else if (state->current_response.ret_code == SUCCESS)
    {
        printf("A operação foi bem-sucedida. payload=%s", state->current_response.payload);
        if (last_syscall_op == DL)
        {
            printf(", allfilenames=[ ");
            for (int i = 0; i < state->current_response.nrnames; i++)
            {
                int start = state->current_response.fstlstpositions[i].start;
                int end = state->current_response.fstlstpositions[i].end;
                int len = end - start + 1;
                printf("%.*s ", len, &state->current_response.allfilenames[start]);
            }
            printf("]");
        }
        printf("\n");
    }
    else if (state->current_response.ret_code == EMPTY)
    {
        printf("A resposta está vazia.\n");
    }

    state->current_response.ret_code = EMPTY;
    strcpy(state->current_response.payload, "");
}

int main(void)
{
    int syscall_fifo = open(SYSCALL_FIFO_PATH, O_WRONLY);
    if (syscall_fifo < 0)
    {
        perror("erro ao abrir fifo");
    }

    pid_t mypid = getpid();

    srand(time(NULL) ^ getpid());
    int shmid = shmget(CORE_STATE_SHMEM_KEY, sizeof(State),
                       IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0)
    {
        perror("Não consegui pegar shmem.");
        exit(EXIT_FAILURE);
    }

    state = (State *)shmat(shmid, 0, 0);

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = A_SLEEP;

    printf("Processo %d: começando pra valer\n", mypid);

    // Syscall inicial DL para popular o state com nomes de arquivos/diretórios
    syscall_args args_init = {.is_shared = 0, .offset = 0, .path = "", .Op = DL, .payload = "", .dir_name = ""};
    syscall_sim(mypid, syscall_fifo, args_init);

    // Pequena pausa para garantir que o kernel processe antes do loop
    nanosleep(&tim, &tim2);
    if (state->current_response.ret_code != EMPTY)
    {
        print_response(mypid, state, args_init.Op);
    }

    for (state->PC = 0; state->PC < MAX; state->PC++)
    {
        nanosleep(&tim, &tim2);
        enum operation_type op = maybe_syscall(mypid, syscall_fifo);
        nanosleep(&tim, &tim2);
        if (op != NO_OPERATION && state->current_response.ret_code != EMPTY)
        {
            print_response(mypid, state, op);
        }
        printf("Processo %d: acabei iteração %d\n", mypid, state->PC);
    }

    printf("Processo %d: acabei tudo!\n", mypid);

    syscall_args args = {.is_shared = 0, .offset = 0, .path = "", .Op = EXIT, .payload = "", .dir_name = ""};

    write(syscall_fifo, &args, sizeof(args)); // fake exit syscall

    return 0;
}
