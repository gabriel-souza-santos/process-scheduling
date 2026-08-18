/**
 * @file engine_simulation.h
 * @brief Motor de simulação de escalonamento de processos orientado a
 *        eventos discretos.
 *
 * O motor consome processos criados via process.h e simula sua execução
 * segundo a política Round-Robin: cada despacho de CPU recebe no máximo
 * `quantum` unidades de tempo; se o processo não concluir sua rajada de
 * CPU dentro do quantum, é preemptado e retorna ao fim da fila de
 * prontos. Rajadas de E/S são processadas por completo assim que
 * iniciadas (não sofrem preempção).
 *
 * O relógio da simulação não avança em passos fixos: a cada iteração do
 * loop principal (engine_run), o motor salta diretamente para o
 * instante do próximo evento pendente (chegada, fim de rajada de CPU/
 * quantum, ou fim de E/S), o que torna a simulação eficiente independente
 * da magnitude das durações envolvidas.
 */

#ifndef ENGINE_SIMULATION_H
#define ENGINE_SIMULATION_H

#include <stdbool.h>
#include <stddef.h>

#include "process.h"

/**
 * @brief Motor de simulação (tipo opaco).
 *
 * Encapsula a fila de eventos, a fila de prontos e as estatísticas de
 * todos os processos cadastrados. Deve ser criado com engine_new() e
 * liberado com engine_destroy().
 */
typedef struct EngineSimulation EngineSimulation;

/**
 * @brief Estatísticas finais coletadas para um processo.
 *
 * Preenchidas ao longo de engine_run() e disponíveis para consulta após
 * o término da simulação, via engine_get_stats() ou engine_print_report().
 */
typedef struct {
  int id; /**< Identificador do processo (igual a process_id()). */
  Duration arrival_time;    /**< Instante de chegada ao sistema. */
  Duration finish_time;     /**< Instante em que o processo terminou (estado
                               PROCESS_TERMINATED). */
  Duration waiting_time;    /**< Soma de todo o tempo que o processo passou no
                               estado PROCESS_READY. */
  Duration turnaround_time; /**< Tempo total no sistema: finish_time -
                               arrival_time. */
} ProcessStats;

/**
 * @brief Cria um novo motor de simulação, vazio.
 *
 * @param quantum Fatia de tempo de CPU concedida a cada despacho
 *                (escalonamento Round-Robin). Um processo que não
 *                termine sua rajada de CPU dentro do quantum é
 *                preemptado e volta para o fim da fila de prontos.
 *                Use um valor muito alto (ex.: SIZE_MAX) para simular
 *                FCFS (sem preempção por quantum).
 *
 * @return Ponteiro para o motor recém-criado, ou NULL em caso de falha
 *         de alocação.
 */
EngineSimulation *engine_new(Duration quantum);

/**
 * @brief Destrói o motor e todos os processos nele cadastrados.
 *
 * Libera todos os recursos internos do motor e chama process_destroy()
 * para cada processo adicionado via engine_add_process(). Após a
 * chamada, `*engine` é definido como NULL.
 *
 * @param engine Ponteiro para o ponteiro do motor a ser destruído.
 *               Chamadas com `engine == NULL` ou `*engine == NULL` são
 *               ignoradas com segurança.
 */
void engine_destroy(EngineSimulation **engine);

/**
 * @brief Adiciona um processo à simulação.
 *
 * Deve ser chamado antes de engine_run(); adicionar processos após o
 * início da simulação não tem efeito sobre uma execução já em curso.
 *
 * @warning A partir desta chamada, o motor passa a ser dono do
 *          ponteiro `p` e é responsável por destruí-lo (via
 *          engine_destroy()). Não chame process_destroy() manualmente
 *          sobre um processo já adicionado ao motor.
 *
 * @param engine Motor de destino.
 * @param p Processo a ser adicionado (criado previamente com process_new()).
 *
 * @return true se o processo foi adicionado com sucesso; false se
 *         `engine` ou `p` forem NULL, ou se houver falha de alocação.
 */
bool engine_add_process(EngineSimulation *engine, Process *p);

/**
 * @brief Executa o loop principal da simulação.
 *
 * Processa, em ordem cronológica, os eventos de chegada, fim de rajada
 * de CPU (ou fim de quantum) e fim de rajada de E/S de todos os
 * processos cadastrados, despachando processos prontos para a CPU
 * segundo a política Round-Robin, até que todos os processos tenham
 * terminado (estado PROCESS_TERMINATED). Ao final, as estatísticas de
 * cada processo ficam disponíveis via engine_get_stats() /
 * engine_print_report().
 *
 * @param engine Motor a ser executado. Se `engine` for NULL ou não
 *               possuir processos cadastrados, a chamada não tem efeito.
 */
void engine_run(EngineSimulation *engine);

/**
 * @brief Retorna o tempo total decorrido de simulação.
 *
 * Válido após engine_run(); representa o instante em que o último
 * evento da simulação foi processado (ou seja, o tempo em que o
 * último processo terminou).
 *
 * @param engine Motor consultado.
 * @return Tempo de simulação decorrido, ou 0 se `engine` for NULL.
 */
Duration engine_current_time(EngineSimulation *engine);

/**
 * @brief Retorna a quantidade de processos cadastrados no motor.
 *
 * @param engine Motor consultado.
 * @return Número de processos adicionados via engine_add_process(),
 *         ou 0 se `engine` for NULL.
 */
size_t engine_process_count(EngineSimulation *engine);

/**
 * @brief Obtém as estatísticas finais de um processo específico.
 *
 * @param engine Motor consultado (deve já ter executado engine_run()).
 * @param process_id Identificador do processo (process_id()).
 * @param[out] out Estrutura preenchida com as estatísticas encontradas.
 *
 * @return true se um processo com o id informado foi encontrado e
 *         `*out` foi preenchido; false caso contrário (ou se `engine`
 *         ou `out` forem NULL).
 */
bool engine_get_stats(EngineSimulation *engine, int process_id,
                      ProcessStats *out);

/**
 * @brief Imprime no stdout um relatório tabular com as estatísticas de
 *        cada processo, além das médias de espera e turnaround da
 *        simulação.
 *
 * @param engine Motor consultado (deve já ter executado engine_run()).
 *               Chamadas com `engine == NULL` são ignoradas.
 */
void engine_print_report(EngineSimulation *engine);

#endif /* ENGINE_SIMULATION_H */
