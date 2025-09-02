#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE* arq_msg_dia = fopen("msg_dia.txt","r");
    if (arq_msg_dia == NULL) {
        printf("Erro ao abrir o arquivo msg_dia.txt\n");
        return 1;
    }
    char linha[256];
    if (fgets(linha, sizeof(linha), arq_msg_dia) == 0) {
        printf("Erro lendo txt\n");
        fclose(arq_msg_dia);
        return 1;
    }

    size_t tam = strlen(linha) + 1;

    int shmid = shmget(8752, tam, IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

    if (shmid < 0) {
        printf("Erro ao criar a memoria compartilhada\n");
        fclose(arq_msg_dia);
        return 1;
    }
    char* msg = (char*) shmat(shmid, 0, 0);

    strcpy(msg, linha);

    shmdt(msg);

    fclose(arq_msg_dia);
    return 0;
}