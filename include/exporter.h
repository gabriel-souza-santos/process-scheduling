#ifndef EXPORTER_H
#define EXPORTER_H

#include <stddef.h>
#include "scheduler.h"

typedef struct {
    const char *scenario_id;             // Ex: "cenario_1_aleatorio_equilibrado"
    size_t num_processes;
    size_t num_seeds;
    SchedulerMetrics *metrics_per_seed;  // Array com 'num_seeds' métricas
} ScenarioData;

// Exporta o arquivo JSON para o algoritmo informado
int export_algorithm_metrics_json(const char *filepath, const char *algo_name, const ScenarioData scenarios[], size_t scenario_count);

#endif // EXPORTER_H