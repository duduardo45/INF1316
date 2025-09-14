#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  pid_t pid;

  pid_t pids[3] = {-1, -1, -1};

  for (int i = 0; i < 3; i++) {
    if ((pid = fork()) < 0) {
      fprintf(stderr, "Erro ao criar filho\n");
      exit(-1);
    }
    if (pid == 0) { /* child */
      switch (i) {
      case 0:
        execv("./build/exc7_1", NULL);
        break;
      case 1:
        execv("./build/exc7_2", NULL);
        break;
      case 2:
        execv("./build/exc7_3", NULL);
        break;
      }
    }

    kill(pid, SIGSTOP);
    pids[i] = pid;
  }

  while (1) {
    for (int i = 0; i < 3; i++) {
      short slice_size = 1;
      if (i != 0) {
        slice_size = 2;
      }

      printf("Hora do %d rodar por %d segundos\n", i + 1, slice_size);
      for (int j = 0; j < 3; j++) {
        if (j == i) {
          kill(pids[j], SIGCONT);
        } else {
          kill(pids[j], SIGSTOP);
        }
      }
      sleep(slice_size);
    }
  }

  return 0;
}
