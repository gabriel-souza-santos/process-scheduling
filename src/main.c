#include <stdio.h>
#include <time.h>

#include "seed_generator.h"

void seed_test(void) {
  /* utiliza o horário atual como seed */
  rng_seed((unsigned int)time(NULL));

  printf("Numeros aleatorios:\n");

  for (int i = 0; i < 10; i++) {
    printf("%d\n", rng_between(1, 100));
  }
}

int main(void) {

  seed_test();

  return 0;
}
