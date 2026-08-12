#ifndef SEED_GENERATOR_H
#define SEED_GENERATOR_H

/**
 * @brief Inicializa o gerador de números pseudoaleatórios.
 *
 * Define o valor inicial da seed utilizado pelo gerador.
 * A mesma seed produzirá sempre a mesma sequência de números.
 *
 * @param seed Valor inicial da sequência pseudoaleatória.
 */
void rng_seed(unsigned int seed);

/**
 * @brief Gera o próximo número pseudoaleatório da sequência.
 *
 * Utiliza um algoritmo Linear Congruential Generator (LCG)
 * para atualizar o estado interno e produzir um novo valor.
 *
 * @return Próximo número pseudoaleatório gerado.
 */
unsigned int rng_next(void);

/**
 * @brief Gera um número pseudoaleatório dentro de um intervalo.
 *
 * Retorna um valor inteiro entre min e max, inclusive.
 *
 * @param min Limite inferior do intervalo.
 * @param max Limite superior do intervalo.
 *
 * @return Número pseudoaleatório no intervalo [min, max].
 */
int rng_between(int min, int max);

#endif /* SEED_GENERATOR_H */
