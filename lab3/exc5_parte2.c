#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void floating_point_exception_handler(int signal);

int main(void) {
  signal(SIGFPE, floating_point_exception_handler);

  int num1, num2;
  printf("Digite num1: ");
  scanf("%d", &num1);
  printf("Digite num2: ");
  scanf("%d", &num2);

  printf("Soma:%d\n", num1 + num2);
  printf("Subtração:%d\n", num1 - num2);
  printf("Multiplicação:%d\n", num1 * num2);
  printf("Divisão:%d\n", num1 / num2);
}

void floating_point_exception_handler(int signal) {
  printf("Ah seu malandro, achou que ia passar 0 pro num2 né? Eu tô "
         "preparado!\n");
  exit(EXIT_FAILURE);
}
