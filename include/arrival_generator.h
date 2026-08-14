/**
 * @file arrival_generator.h
 * @brief Geração determinística dos tempos de chegada dos processos
 *        (seção 4 e 5 do enunciado do projeto).
 *
 * Este módulo é responsável apenas por decidir "quando cada processo
 * chega ao sistema". A geração dos demais atributos do processo
 * (prioridade, rajadas de CPU/E-S, etc.) fica a cargo de outro gerador
 * de carga de trabalho, que deve reutilizar o mesmo stream do
 * seed_generator (ver observação de determinismo mais abaixo).
 *
 * Modelos suportados (todos documentados no relatório, conforme exigido
 * pelo enunciado):
 *
 *  - ARRIVAL_ALL_AT_ZERO
 *      Todos os processos chegam no instante 0.
 *      Uso permitido apenas com justificativa explícita no relatório:
 *      é útil para isolar o efeito do algoritmo de escalonamento em si
 *      (sem o ruído introduzido pela ordem/dispersão de chegadas), mas
 *      é uma limitação, pois não representa um sistema realista onde
 *      processos surgem ao longo do tempo. Nesse modelo a fila de
 *      prontos fica populada por completo já no instante 0, então
 *      métricas como turnaround tendem a ser dominadas pela ordem de
 *      desempate do escalonador, e não pela dinâmica de chegadas.
 *
 *  - ARRIVAL_FIXED_INTERVAL
 *      Chegadas espaçadas por um intervalo fixo e determinístico:
 *      arrival[i] = i * fixed_interval. Não usa o RNG. Útil para
 *      cenários de controle/depuração, onde se quer eliminar a
 *      variância de chegada e comparar algoritmos "no mesmo terreno".
 *
 *  - ARRIVAL_RANDOM_INTERVAL
 *      Modelo padrão recomendado para os cenários obrigatórios. O
 *      primeiro processo chega em t=0; cada processo seguinte chega
 *      `gap` unidades de tempo depois do anterior, com
 *      gap ~ Uniforme[min_interval, max_interval] (inclusive),
 *      amostrado via seed_generator (rng_between). Isso aproxima
 *      chegadas espalhadas ao longo do tempo, como em um sistema real,
 *      mantendo total controle estatístico via seed.
 *
 *  - ARRIVAL_BATCH
 *      Processos chegam em lotes de `batch_size` processos
 *      simultâneos; lotes consecutivos são espaçados por
 *      `batch_interval`. Modela cargas em que múltiplos processos são
 *      disparados juntos (ex.: início de um turno de processamento em
 *      lote, um cron que dispara N jobs de uma vez).
 *
 * Em todos os modelos, para uma mesma seed e mesmos parâmetros de
 * configuração, a sequência gerada é sempre idêntica (requisito da
 * seção 5: "a mesma seed, no mesmo cenário, deve gerar exatamente a
 * mesma carga de trabalho").
 *
 * @note Determinismo e ordem de chamadas: os modelos ARRIVAL_RANDOM_INTERVAL
 *       e ARRIVAL_BATCH* consomem números do gerador global de
 *       seed_generator.h. arrival_generate() sempre chama rng_seed()
 *       com a seed informada em ArrivalConfig antes de consumir
 *       números, portanto o resultado não depende de chamadas
 *       anteriores ao RNG. Isso também significa que, ao integrar este
 *       módulo com o restante do gerador de carga (prioridade, rajadas
 *       de CPU/E-S), a geração dos tempos de chegada deve ser a
 *       *primeira* etapa do pipeline para aquela seed — chamadas
 *       subsequentes que também usem rng_between() continuarão a
 *       sequência a partir daí, de forma determinística.
 *
 * (*) ARRIVAL_BATCH também é determinístico, mas atualmente não
 *     consome números aleatórios (o tamanho do lote e o intervalo são
 *     fixos); caso variações aleatórias de tamanho de lote sejam
 *     adicionadas no futuro, este comentário deve ser atualizado.
 */

#ifndef ARRIVAL_GENERATOR_H
#define ARRIVAL_GENERATOR_H

#include <stddef.h>

#include "process.h"

/** @brief Modelo de chegada dos processos. */
typedef enum {
  ARRIVAL_ALL_AT_ZERO,     /**< Todos os processos chegam em t=0. */
  ARRIVAL_FIXED_INTERVAL,  /**< Chegadas em intervalos fixos e regulares. */
  ARRIVAL_RANDOM_INTERVAL, /**< Chegadas em intervalos aleatórios (uniforme). */
  ARRIVAL_BATCH            /**< Chegadas em lotes espaçados regularmente. */
} ArrivalModel;

/**
 * @brief Configuração do gerador de tempos de chegada.
 *
 * Apenas os campos relevantes para o `model` escolhido precisam ser
 * preenchidos; os demais são ignorados. Use arrival_config_validate()
 * para checar a consistência antes de chamar arrival_generate().
 */
typedef struct {
  ArrivalModel model;
  unsigned int seed; /**< Seed usada para (re)inicializar o RNG global. */

  /* ARRIVAL_FIXED_INTERVAL */
  Duration fixed_interval; /**< Intervalo entre chegadas consecutivas. */

  /* ARRIVAL_RANDOM_INTERVAL */
  Duration min_interval; /**< Intervalo mínimo (inclusive) entre chegadas. */
  Duration max_interval; /**< Intervalo máximo (inclusive) entre chegadas. */

  /* ARRIVAL_BATCH */
  size_t batch_size; /**< Quantidade de processos por lote (>= 1). */
  Duration
      batch_interval; /**< Intervalo entre o início de lotes consecutivos. */
} ArrivalConfig;

/**
 * @brief Valida se uma configuração é consistente para o modelo escolhido.
 *
 * Verificações realizadas:
 *  - ARRIVAL_FIXED_INTERVAL: nenhuma restrição adicional (intervalo 0 é
 *    equivalente a ARRIVAL_ALL_AT_ZERO e é aceito).
 *  - ARRIVAL_RANDOM_INTERVAL: min_interval <= max_interval.
 *  - ARRIVAL_BATCH: batch_size >= 1.
 *
 * @param config Configuração a validar.
 * @return true se `config` for consistente; false caso contrário (ou se
 *         `config` for NULL, ou `model` for inválido).
 */
bool arrival_config_validate(const ArrivalConfig *config);

/**
 * @brief Gera os tempos de chegada para `process_count` processos.
 *
 * (Re)inicializa o gerador de números pseudoaleatórios global
 * (seed_generator.h) com `config->seed` antes de gerar qualquer valor,
 * garantindo que a mesma configuração sempre produza a mesma sequência
 * de tempos de chegada, independentemente de chamadas anteriores ao
 * RNG (ver nota de determinismo em arrival_generator.h).
 *
 * O array retornado está sempre em ordem não decrescente
 * (arrival[i] <= arrival[i+1]), refletindo a ordem em que os
 * processos passam a existir na simulação.
 *
 * @param config Configuração do modelo de chegada. Deve satisfazer
 *               arrival_config_validate(config).
 * @param process_count Quantidade de processos para os quais gerar
 *                       tempos de chegada. Se 0, retorna NULL.
 *
 * @return Array alocado dinamicamente com `process_count` elementos
 *         (liberar com free() pelo chamador), ou NULL em caso de
 *         configuração inválida, `process_count == 0` ou falha de
 *         alocação.
 */
Duration *arrival_generate(const ArrivalConfig *config, size_t process_count);

#endif /* ARRIVAL_GENERATOR_H */
