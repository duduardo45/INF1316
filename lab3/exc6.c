#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void usr1_handler(int signal);
void usr2_handler(int signal);

time_t call_beginning = 0;

int main(void) {
  signal(SIGUSR1, usr1_handler);
  signal(SIGUSR2, usr2_handler);

  printf("Hora de monitorar chamadas!\n");
  while (1) {
    pause();
  }
}

void usr1_handler(int signal) { // chamada começou
  time(&call_beginning);
}

void usr2_handler(int signal) { // chamada terminou
  if (call_beginning == 0) {
    printf("Tem que começar chamada pra poder terminar.\n");
  }

  time_t duration = time(NULL) - call_beginning;

  int cents;
  if (duration <= 60) {
    cents = 2 * duration;
  } else {
    cents = 60 * 2 + (duration - 60);
  }

  printf("Sua ligação custou R$%d,%02d\n", cents / 100, cents % 100);
}
