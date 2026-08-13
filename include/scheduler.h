#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include "process.h"

/* -----------------------------------------------------------------------
 * Resultado de uma simulação de escalonamento
 * ----------------------------------------------------------------------- */

/* Métricas calculadas ao final da simulação para cada processo */
typedef struct {
    int process_id;
    Duration arrival_time;
    Duration completion_time; /* instante em que o processo terminou */
    Duration turnaround_time; /* completion_time - arrival_time */
    Duration waiting_time;    /* turnaround_time - tempo total de CPU */
    Duration response_time;   /* instante da 1ª execução - arrival_time */
} ProcessMetrics;

/* Resultado global da simulação */
typedef struct {
    ProcessMetrics *metrics; /* array de métricas, um por processo */
    size_t count;            /* número de processos simulados */
    Duration total_time;     /* tempo total decorrido na simulação */
    double avg_turnaround;
    double avg_waiting;
    double avg_response;
    double cpu_utilization;  /* fração do tempo total com CPU ocupada */
} SchedulerResult;

void scheduler_result_destroy(SchedulerResult *r);

/* Imprime uma tabela formatada com as métricas de cada processo
 * e as médias globais da simulação. */
void scheduler_result_print(const SchedulerResult *r, const char *algorithm_name);

/* -----------------------------------------------------------------------
 * Algoritmos de Escalonamento
 * ----------------------------------------------------------------------- */

/*
 * FCFS — First Come, First Served (não preemptivo)
 *
 * Executa os processos na ordem de chegada. Cada processo ocupa a CPU
 * até completar toda a sua rajada de CPU atual antes de passar para o
 * próximo. Processos em E/S são avançados concorrentemente.
 *
 * @param processes  Array de ponteiros para os processos a escalonar.
 * @param count      Número de processos no array.
 * @return           SchedulerResult com as métricas da simulação.
 *                   O chamador é responsável por liberar com
 *                   scheduler_result_destroy().
 */
SchedulerResult scheduler_fcfs(Process **processes, size_t count);

/*
 * Round Robin (preemptivo por quantum)
 *
 * Cada processo recebe no máximo `quantum` unidades de tempo na CPU.
 * Ao esgotar o quantum, o processo é reinserido no final da fila de
 * prontos e o próximo processo é selecionado.
 *
 * @param processes  Array de ponteiros para os processos a escalonar.
 * @param count      Número de processos no array.
 * @param quantum    Fatia de tempo máxima por execução.
 * @return           SchedulerResult com as métricas da simulação.
 */
SchedulerResult scheduler_round_robin(Process **processes, size_t count,
                                      Duration quantum);

/*
 * Escalonamento por Prioridade Não Preemptivo
 *
 * A cada vez que a CPU fica livre, escolhe o processo de maior
 * prioridade dentre os que já chegaram. Em caso de empate na
 * prioridade, usa a ordem de chegada (FCFS).
 * O campo `priority` em ProcessAttr segue a convenção: quanto maior
 * o valor, maior a prioridade.
 *
 * @param processes  Array de ponteiros para os processos a escalonar.
 * @param count      Número de processos no array.
 * @return           SchedulerResult com as métricas da simulação.
 */
SchedulerResult scheduler_priority(Process **processes, size_t count);

/* -----------------------------------------------------------------------
 * Funções auxiliares (TODO: implementar conforme necessário)
 * ----------------------------------------------------------------------- */

/* TODO: Retorna o índice do processo com menor tempo de chegada no array */
size_t scheduler_earliest_arrival(Process **processes, size_t count);

/* TODO: Calcula o tempo médio de espera ponderado pela prioridade */
double scheduler_weighted_avg_waiting(const SchedulerResult *r,
                                      Process **processes, size_t count);

#endif /* SCHEDULER_H */
