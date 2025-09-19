#include <stdlib.h>
#include <unistd.h>

#define MAX 100

int main(int argc, char *argv[]) {
  while (PC < MAX) {
    sleep(0.5);
    int d = ((rand() % 100) + 1);
    if (d < 15) { // generate a random syscall
      if (d % 2)
        Dx = D1 else Dx = D2;
      if (d % 3 == 1)
        Op = R else if (d % 3 = 1) Op = W else Op = X;
      syscall()
    }
    sleep(0.5);
  }
}
