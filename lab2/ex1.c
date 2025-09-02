#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void printa_matriz(int* matriz, int linhas, int colunas);

int main(int argc, char* argv[])
{
    int mypid, pid, status, q=0;

    mypid = getpid();

    if (argc != 3)
    {
        printf("Uso: %s <colunas> <linhas>\n", argv[0]);
        exit(1);
    }

    int colunas = atoi(argv[1]);
    int linhas = atoi(argv[2]);

    int qtd_nums = colunas*linhas;

    int shmid1 = shmget(IPC_PRIVATE, qtd_nums*sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR);
    int shmid2 = shmget(IPC_PRIVATE, qtd_nums*sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR);
    int shmid3 = shmget(IPC_PRIVATE, qtd_nums*sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR);

    int* matriz1 = (int*)shmat(shmid1, 0, 0);
    int* matriz2 = (int*)shmat(shmid2, 0, 0);
    int* matriz3 = (int*)shmat(shmid3, 0, 0);

    for (int i = 0; i < 2*qtd_nums; i++) {
        if (i%2) {
            matriz2[i/2] = i;
        } else {
            matriz1[i/2] = i;
        }
    }

    for (int i = 0; i<linhas; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Erro no fork\n");
            exit(1);
        }
        if (pid == 0) {
            for (int j = 0; j<colunas; j++) {
                int idx = i*colunas + j;
                matriz3[idx] = matriz1[idx] + matriz2[idx];
            }
            shmdt(matriz1);
            shmdt(matriz2);
            shmdt(matriz3);
            exit(0);
        }
    }

    for (int i = 0; i<linhas; i++) wait(&status);

    printa_matriz(matriz1, linhas, colunas);
    printa_matriz(matriz2, linhas, colunas);
    printa_matriz(matriz3, linhas, colunas);

    shmdt(matriz1);
    shmdt(matriz2);
    shmdt(matriz3);

    shmctl (shmid1, IPC_RMID, 0);
    shmctl (shmid2, IPC_RMID, 0);
    shmctl (shmid3, IPC_RMID, 0);

    return 0;
}

void printa_matriz(int* matriz, int linhas, int colunas) {
    printf("Matriz:\n");
    for (int j = 0; j<colunas; j++) {
        printf("--------");
    }
    printf("--------\n");
    for (int i = 0; i<linhas; i++) {
        printf("|\t");
        for (int j = 0; j<colunas; j++) {
            int idx = i*colunas + j;
            printf("%d\t", matriz[idx]);
        }
        printf("|\n");
    }
    for (int j = 0; j<colunas; j++) {
        printf("--------");
    }
    printf("--------\n");
}