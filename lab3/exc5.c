#include <stdio.h>

int main(void) {
  int num1, num2;
  printf("Digite num1:");
  scanf("%d", &num1);
  printf("Digite num2:");
  scanf("%d", &num2);

  printf("Soma:%d\n", num1 + num2);
  printf("Subtração:%d\n", num1 - num2);
  printf("Multiplicação:%d\n", num1 * num2);
  printf("Divisão:%d\n", num1 / num2);
}
