#include "seed_generator.h"

/* Armazena o estado atual do gerador.
 * Cada novo número gerado atualiza esse valor.
 */
static unsigned int rng_state;

/* Inicializa o gerador com uma seed fornecida pelo usuário.
 * A mesma seed sempre ira produzir a mesma sequência de números.
 */
void rng_seed(unsigned int seed) { rng_state = seed; }

unsigned int rng_next(void) {
  /* Gera o próximo número da sequência.
   *
   * O valor atual (rng_state) é usado para calcular
   * um novo valor através de uma fórmula matemática.
   *
   * Cada número gerado depende do número anterior,
   * criando uma sequência que parece aleatória.
   *
   * Exemplo:
   * Seed = 10
   * -> 1030549473
   * -> 797165516
   * -> 2863095995
   * -> ...
   */
  rng_state = rng_state * 1664525u + 1013904223u;

  /* Salva e retorna o novo número gerado. */
  return rng_state;
}
int rng_between(int min, int max) {
  /* Gera um número pseudoaleatório qualquer. */
  unsigned int random = rng_next();

  /* Limita o valor para o intervalo desejado.
   *
   * Exemplo:
   * min = 1, max = 10
   * intervalo = 10
   * random % 10 -> valor entre 0 e 9
   * + min -> valor entre 1 e 10
   */
  return min + (random % (max - min + 1));
}
