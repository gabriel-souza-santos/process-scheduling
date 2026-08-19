#include "exporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double mean;
    double std;
} MetricStats;

static MetricStats calculate_stats(const double values[], size_t count) {
    MetricStats stats = {0.0, 0.0};
    if (count == 0) return stats;

    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += values[i];
    }
    stats.mean = sum / (double)count;

    if (count > 1) {
        double variance_sum = 0.0;
        for (size_t i = 0; i < count; i++) {
            double diff = values[i] - stats.mean;
            variance_sum += diff * diff;
        }
        stats.std = sqrt(variance_sum / (double)(count - 1)); // Desvio padrão amostral
    }

    return stats;
}

int export_algorithm_metrics_json(const char *filepath, const char *algo_name, const ScenarioData scenarios[], size_t scenario_count) {
    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Erro ao abrir arquivo para escrita do JSON");
        return -1;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"algorithm\": \"%s\",\n", algo_name);
    fprintf(file, "  \"scenarios\": {\n");

    for (size_t s = 0; s < scenario_count; s++) {
        const ScenarioData *sc = &scenarios[s];
        size_t n = sc->num_seeds;

        // Aloca vetores temporários para extrair e calcular a estatística de cada métrica
        double *turnarounds = malloc(n * sizeof(double));
        double *slowdowns   = malloc(n * sizeof(double));
        double *switches    = malloc(n * sizeof(double));
        double *times       = malloc(n * sizeof(double));
        double *jains       = malloc(n * sizeof(double));

        for (size_t i = 0; i < n; i++) {
            turnarounds[i] = sc->metrics_per_seed[i].average_turnaround;
            slowdowns[i]   = sc->metrics_per_seed[i].average_slowdown;
            switches[i]    = sc->metrics_per_seed[i].context_switches;
            times[i]       = sc->metrics_per_seed[i].total_time;
            jains[i]       = sc->metrics_per_seed[i].jain_index;
        }

        MetricStats st_turnaround = calculate_stats(turnarounds, n);
        MetricStats st_slowdown   = calculate_stats(slowdowns, n);
        MetricStats st_switches   = calculate_stats(switches, n);
        MetricStats st_time       = calculate_stats(times, n);
        MetricStats st_jain       = calculate_stats(jains, n);

        free(turnarounds);
        free(slowdowns);
        free(switches);
        free(times);
        free(jains);

        // Formatação JSON do Cenário
        fprintf(file, "    \"%s\": {\n", sc->scenario_id);
        fprintf(file, "      \"num_processes\": %zu,\n", sc->num_processes);
        fprintf(file, "      \"num_seeds\": %zu,\n", sc->num_seeds);
        fprintf(file, "      \"metrics\": {\n");
        fprintf(file, "        \"turnaround\":        { \"mean\": %.4f, \"std\": %.4f },\n", st_turnaround.mean, st_turnaround.std);
        fprintf(file, "        \"slowdown\":          { \"mean\": %.4f, \"std\": %.4f },\n", st_slowdown.mean, st_slowdown.std);
        fprintf(file, "        \"context_switches\":  { \"mean\": %.2f, \"std\": %.2f },\n", st_switches.mean, st_switches.std);
        fprintf(file, "        \"total_time\":        { \"mean\": %.2f, \"std\": %.2f },\n", st_time.mean, st_time.std);
        fprintf(file, "        \"jain_index\":        { \"mean\": %.4f, \"std\": %.4f }\n", st_jain.mean, st_jain.std);
        fprintf(file, "      }\n");
        fprintf(file, "    }%s\n", (s < scenario_count - 1) ? "," : "");
    }

    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return 0;
}