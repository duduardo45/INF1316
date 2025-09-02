#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    
    int pid = fork();
    if (pid != 0) {
        wait(NULL);

        int shmid_tam = shmget(8753, sizeof(size_t), S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);

        size_t* tam = (size_t*) shmat(shmid_tam, 0, 0);


        int shmid = shmget(8752, *tam, S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
        if (shmid < 0) {
            printf("Erro ao criar a memoria compartilhada\n");
            return 1;
        }
        char* msg = (char*) shmat(shmid, 0, 0);

        shmdt(tam);
        shmctl(shmid_tam, IPC_RMID, 0);


        printf("Mensagem do dia: %s\n", msg);
        shmdt(msg);
        shmctl(shmid, IPC_RMID, 0);
    } else {
        execl("./build/le_msg_dia", "le_msg_dia", NULL);
        printf("Erro ao executar o programa le_msg_dia\n");
        exit(1);
    }


}