#include <stdio.h>
#include "scheduler.h"

/*
 * Exibe as metricas de um algoritmo de escalonamento no stdout.
 */
void metrics_print(SchedulerMetrics m, const char *algorithm_name) {
    printf("\n");
    printf("    %s    \n", algorithm_name);
    printf("--------------------\n");
    printf("  Processos simulados  : %zu\n",   m.num_processes);
    printf("  Tempo total          : %zu\n",   m.total_time);
    printf("  Trocas de contexto   : %zu\n",   m.context_switches);
    printf("  Comparações da Fila  : %zu\n",   m.total_comparisons);
    printf("  Turnaround medio     : %.4f\n",  m.average_turnaround);
    printf("  Slowdown medio       : %.4f\n",  m.average_slowdown);
    printf("  Indice de Jain       : %.4f\n",  m.jain_index);
}

void metrics_export(const char *filepath, SchedulerMetrics m,
                    const char *algorithm_name, const char *scenario) {
    // TODO
}