#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define MAX_CHAMADAS 100

void usr1_handler(int signal);
void usr2_handler(int signal);

unsigned int qtd_chamadas = 0;
time_t call_beginning[MAX_CHAMADAS];

int main(void) {
  signal(SIGUSR1, usr1_handler);
  signal(SIGUSR2, usr2_handler);

  printf("Hora de monitorar chamadas!\n");
  while (1) {
    pause();
  }
}

void usr1_handler(int signal) { // chamada começou
  if (qtd_chamadas >= MAX_CHAMADAS) {
    printf("Número máximo de chamadas atingido.\n");
    return;
  }
  qtd_chamadas++;
  time(&(call_beginning[qtd_chamadas - 1]));
  printf("Chamada %d começou!\n", qtd_chamadas);
}

void usr2_handler(int signal) { // chamada terminou
  if (qtd_chamadas == 0) {
    printf("Tem que começar uma chamada pra poder terminar.\n");
    return;
  }

  time_t duration = time(NULL) - call_beginning[qtd_chamadas - 1];

  int cents;
  if (duration <= 60) {
    cents = 2 * duration;
  } else {
    cents = 60 * 2 + (duration - 60);
  }

  printf("A ligação %d custou R$%d,%02d\n",qtd_chamadas, cents / 100, cents % 100);
  qtd_chamadas--;
}
