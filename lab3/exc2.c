#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void intHandler(int sinal);
void killHandler(int sinal);

int main(void) {
  void (*p)(int); // ponteiro para função que recebe int como
  // parâmetro
  p = signal(SIGINT, intHandler);
  printf("Endereco do manipulador anterior %p\n", p);
  p = signal(SIGKILL, killHandler);
  printf("Endereco do manipulador anterior %p\n", p);
  puts("Ctrl-C desabilitado. Use Ctrl-\\ para terminar");
  for (;;)
    ;
}
void intHandler(int sinal) { printf("Você pressionou Ctrl-C (%d) \n", sinal); }
void killHandler(int sinal) {
  printf("Matando o processo...\n");
  exit(0);
}
