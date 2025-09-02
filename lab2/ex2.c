#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    """ 
    shmget (area do tamanho da proxima)
    shmclose()
    shmget(area do tamanho exato da msg do dia)
))): não pode, disse o monitor. Trise, eu fiquei.
    """
    int shmid = shmget(8752, tam, IPC_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH);
    if (shmid < 0) {
        printf("Erro ao criar a memoria compartilhada\n");
        return 1;
    }
    char* msg = (char*) shmat(shmid, 0, 0);

    if (int id = fork() != 0) {
        wait(NULL);
        printf("Mensagem do dia: %s\n", msg);
        shmdt(msg);
        shmctl(shmid, IPC_RMID, 0);
    } else {
        execl("./le_msg_dia", "le_msg_dia", NULL);
        printf("Erro ao executar o programa le_msg_dia\n");
        shmdt(msg);
        exit(1);
    }
}