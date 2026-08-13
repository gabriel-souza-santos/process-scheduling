#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "process.h"
#include "scheduler.h"
#include "seed_generator.h"

/* -----------------------------------------------------------------------
 * Configuração da simulação
 * ----------------------------------------------------------------------- */
#define NUM_PROCESSES   5
#define QUANTUM         3   /* fatia de tempo para Round Robin */
#define MAX_BURST_COUNT 3   /* número de rajadas por processo (CPU+IO alternados) */
#define MAX_BURST_DUR   10  /* duração máxima de cada rajada */
#define MAX_ARRIVAL     10  /* tempo máximo de chegada */


int main(void) {
    /* Seed fixa para resultados reprodutíveis */
    rng_seed(42);

    /* Cria os processos originais e captura seus parâmetros */
    size_t count = NUM_PROCESSES;

    /* Gera parâmetros manualmente para poder clonar depois */
    Duration arrivals[NUM_PROCESSES];
    int priorities[NUM_PROCESSES];
    Duration *burst_durations[NUM_PROCESSES];
    size_t burst_counts[NUM_PROCESSES];

    printf("=== Simulação de Algoritmos de Escalonamento ===\n\n");
    printf("Processos criados:\n");
    printf("%-6s %-10s %-10s %-25s\n",
           "PID", "Chegada", "Prioridade", "Rajadas (CPU/IO/...)");
    printf("%-6s %-10s %-10s %-25s\n",
           "---", "-------", "----------", "-------------------");

    for (size_t i = 0; i < count; i++) {
        int n_bursts = rng_between(1, MAX_BURST_COUNT) * 2 - 1;

        arrivals[i] = (Duration)rng_between(0, MAX_ARRIVAL);
        priorities[i]=  rng_between(1, 5);
        burst_counts[i] = (size_t)n_bursts;
        burst_durations[i] = malloc(burst_counts[i] * sizeof(Duration));

        printf("P%-5zu %-10zu %-10d  [", i + 1, arrivals[i], priorities[i]);

        for (size_t j = 0; j < burst_counts[i]; j++) {
            burst_durations[i][j] = (Duration)rng_between(1, MAX_BURST_DUR);
            printf("%zu", burst_durations[i][j]);

            if (j < burst_counts[i] - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    }
    printf("\n");

    /* ----------------------------------------------------------------
     * FCFS
     * ---------------------------------------------------------------- */
    Process **processes_fcfs = malloc(count * sizeof(Process *));
    for (size_t i = 0; i < count; i++) {
        processes_fcfs[i] = process_new((ProcessAttr){ 
            .id = (int)(i + 1),
            .arrival_time = arrivals[i],
            .priority = priorities[i]
        }); 

        for (size_t j = 0; j < burst_counts[i]; j++) {
          process_add_burst(processes_fcfs[i], burst_durations[i][j]);
        }
    }

    SchedulerResult res_fcfs = scheduler_fcfs(processes_fcfs, count);
    scheduler_result_print(&res_fcfs, "FCFS (First Come, First Served)");
    scheduler_result_destroy(&res_fcfs);

    for (size_t i = 0; i < count; i++) process_destroy(&processes_fcfs[i]);
    free(processes_fcfs);

    /* ----------------------------------------------------------------
     * Round Robin
     * ---------------------------------------------------------------- */
    Process **procs_rr = malloc(count * sizeof(Process *));
    for (size_t i = 0; i < count; i++) {
        ProcessAttr a = { .id = (int)(i+1), .arrival_time = arrivals[i],
                          .priority = priorities[i] };
        procs_rr[i] = process_new(a);
        for (size_t b = 0; b < burst_counts[i]; b++)
            process_add_burst(procs_rr[i], burst_durations[i][b]);
    }

    printf("Quantum (Round Robin): %d\n", QUANTUM);
    SchedulerResult res_rr = scheduler_round_robin(procs_rr, count, QUANTUM);
    scheduler_result_print(&res_rr, "Round Robin");
    scheduler_result_destroy(&res_rr);

    for (size_t i = 0; i < count; i++) process_destroy(&procs_rr[i]);
    free(procs_rr);

    /* ----------------------------------------------------------------
     * Prioridade Não Preemptivo
     * ---------------------------------------------------------------- */
    Process **procs_pri = malloc(count * sizeof(Process *));
    for (size_t i = 0; i < count; i++) {
        ProcessAttr a = { .id = (int)(i+1), .arrival_time = arrivals[i],
                          .priority = priorities[i] };
        procs_pri[i] = process_new(a);
        for (size_t b = 0; b < burst_counts[i]; b++)
            process_add_burst(procs_pri[i], burst_durations[i][b]);
    }

    SchedulerResult res_pri = scheduler_priority(procs_pri, count);
    scheduler_result_print(&res_pri, "Prioridade Não Preemptivo");
    scheduler_result_destroy(&res_pri);

    for (size_t i = 0; i < count; i++) process_destroy(&procs_pri[i]);
    free(procs_pri);

    /* Libera buffers de rajadas */
    for (size_t i = 0; i < count; i++) free(burst_durations[i]);

    return 0;
}
