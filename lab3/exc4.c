#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  pid_t pid;

  pid_t pids[2] = {-1, -1};

  for (int i = 0; i < 2; i++) {
    if ((pid = fork()) < 0) {
      fprintf(stderr, "Erro ao criar filho\n");
      exit(-1);
    }
    if (pid == 0) { /* child */
      for (;;) {
        ;
      }
    }
    kill(pid, SIGSTOP);
    pids[i] = pid;
  }

  kill(pids[0], SIGSTOP);

  for (int i = 0; i < 5; i++) {
    printf("Parando 1 e continuando 2...\n");
    kill(pids[0], SIGSTOP);
    kill(pids[1], SIGCONT);
    usleep(10000);
    printf("Parando 2 e continuando 1...\n");
    kill(pids[1], SIGSTOP);
    kill(pids[0], SIGCONT);
    usleep(10000);
  }

  kill(pids[0], SIGKILL);
  kill(pids[1], SIGKILL);

  return 0;
}
