#include <stdio.h>
#include <stdlib.h>
#include "simulation.h"
#include "scheduler.h"
#include "scheduler_fcfs.h"
#include "scheduler_round_robin.h"
#include "scheduler_priority.h"
#include "scheduler_custom.h"
#include "seed_generator.h"
#include "exporter.h" // Inclusão do módulo de exportação

#define NUM_PROCESSES 1000
#define NUM_SEEDS 100

typedef enum {
    SCENARIO_BALANCED,
    SCENARIO_IO_BOUND,
    SCENARIO_CPU_BOUND,
    SCENARIO_UNBALANCED_PRIORITY
} ScenarioType;

void update_incremental_avg(SchedulerMetrics *avg, SchedulerMetrics new_metrics, size_t k) {
    avg->average_turnaround += (new_metrics.average_turnaround - avg->average_turnaround) / k;
    avg->average_slowdown   += (new_metrics.average_slowdown - avg->average_slowdown) / k;
    avg->jain_index         += (new_metrics.jain_index - avg->jain_index) / k;
    avg->context_switches   += (new_metrics.context_switches - avg->context_switches) / k;
    avg->num_processes      += (new_metrics.num_processes - avg->num_processes) / k;
    avg->total_comparisons  += (new_metrics.total_comparisons - avg->total_comparisons) / k;
    avg->total_time         += (new_metrics.total_time - avg->total_time) / k;
}

Process** generate_workload(unsigned int seed, ScenarioType type) {
    rng_seed(seed);
    Process **processes = malloc(NUM_PROCESSES * sizeof(Process*));
    if (!processes) return NULL;
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int priority = rng_between(1, 10);
        
        if (type == SCENARIO_UNBALANCED_PRIORITY) {
            // 85% prioridade baixa (1-3) e 15% prioridade alta (8-10) para provocação de starvation
            if (rng_between(1, 100) <= 85) {
                priority = rng_between(1, 3);
            } else {
                priority = rng_between(8, 10);
            }
        }
        
        size_t num_bursts;
        size_t cpu_min, cpu_max;
        size_t io_min, io_max;
        size_t arrival_min, arrival_max;

        switch (type) {
            case SCENARIO_IO_BOUND:
                num_bursts = rng_between(8, 14) * 2 - 1;   // 15 a 27 rajadas intercaladas (Muitas interrupções)
                cpu_min = 1000; cpu_max = 5000;           // CPU curta
                io_min  = 8000; io_max  = 20000;          // E/S longa
                arrival_min = 0; arrival_max = 15000000; 
                break;

            case SCENARIO_CPU_BOUND:
                num_bursts = rng_between(1, 2) * 2 - 1;   // 1 a 3 rajadas intercaladas (Quase não faz I/O)
                cpu_min = 15000; cpu_max = 40000;         // CPU longa
                io_min  = 500;   io_max  = 2000;          // E/S curta
                arrival_min = 0; arrival_max = 15000000; 
                break;

            case SCENARIO_UNBALANCED_PRIORITY:
            case SCENARIO_BALANCED:
            default:
                num_bursts = rng_between(4, 10) * 2 - 1;   // 7 a 19 rajadas intercaladas (Uso equilibrado)
                cpu_min = 2000; cpu_max = 10000;          
                io_min  = 2000; io_max  = 10000;
                arrival_min = 0; arrival_max = 20000000; 
                break;
        }

        ProcessAttr attr = {
            .id = i,
            .priority = priority,
            .arrival_time = rng_between(arrival_min, arrival_max),
        };
        processes[i] = process_new(attr);
        
        for (size_t j = 0; j < num_bursts; j++) {
            if (j % 2 == 0) {
                process_add_burst(processes[i], rng_between(cpu_min, cpu_max));
            } else {
                process_add_burst(processes[i], rng_between(io_min, io_max));
            }
        }
    }
    return processes;
}

void destroy_workload(Process **processes) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process_destroy(&processes[i]);
    }
    free(processes);
}

// A função agora recebe as estruturas ScenarioData para armazenar os resultados
void run_scenario(ScenarioType type, const char* scenario_name, const char* scenario_id,
                  ScenarioData *sc_fcfs, ScenarioData *sc_rr, 
                  ScenarioData *sc_priority, ScenarioData *sc_custom) {
    
    unsigned int base_seed = 12345;
    printf("\nExecutando Cenário: %s...\n", scenario_name);

    SchedulerMetrics avg_fcfs = {0};
    SchedulerMetrics avg_round_robin = {0};
    SchedulerMetrics avg_priority = {0};
    SchedulerMetrics avg_custom = {0};

    // Inicializa as estruturas do exportador
    sc_fcfs->scenario_id = scenario_id; sc_fcfs->num_processes = NUM_PROCESSES; sc_fcfs->num_seeds = NUM_SEEDS;
    sc_fcfs->metrics_per_seed = malloc(NUM_SEEDS * sizeof(SchedulerMetrics));

    sc_rr->scenario_id = scenario_id; sc_rr->num_processes = NUM_PROCESSES; sc_rr->num_seeds = NUM_SEEDS;
    sc_rr->metrics_per_seed = malloc(NUM_SEEDS * sizeof(SchedulerMetrics));

    sc_priority->scenario_id = scenario_id; sc_priority->num_processes = NUM_PROCESSES; sc_priority->num_seeds = NUM_SEEDS;
    sc_priority->metrics_per_seed = malloc(NUM_SEEDS * sizeof(SchedulerMetrics));

    sc_custom->scenario_id = scenario_id; sc_custom->num_processes = NUM_PROCESSES; sc_custom->num_seeds = NUM_SEEDS;
    sc_custom->metrics_per_seed = malloc(NUM_SEEDS * sizeof(SchedulerMetrics));

    for (int s = 0; s < NUM_SEEDS; s++) {
        unsigned int current_seed = base_seed + s;
        size_t k = s + 1;

        // FCFS
        Process **workload_fcfs = generate_workload(current_seed, type);
        Scheduler *fcfs = fcfs_new();
        SchedulerMetrics metrics_fcfs = simulation_run(workload_fcfs, NUM_PROCESSES, fcfs);
        sc_fcfs->metrics_per_seed[s] = metrics_fcfs; // Salva para o JSON
        update_incremental_avg(&avg_fcfs, metrics_fcfs, k);
        fcfs_destroy(fcfs);
        destroy_workload(workload_fcfs);
        
        // Round Robin (usando o quantum atualizado para 10000 ns)
        Process **workload_rr = generate_workload(current_seed, type);
        Scheduler *round_robin = round_robin_new(4000); 
        SchedulerMetrics metrics_rr = simulation_run(workload_rr, NUM_PROCESSES, round_robin);
        sc_rr->metrics_per_seed[s] = metrics_rr; // Salva para o JSON
        update_incremental_avg(&avg_round_robin, metrics_rr, k);
        round_robin_destroy(round_robin);
        destroy_workload(workload_rr);
        
        // Priority
        Process **workload_priority = generate_workload(current_seed, type);
        Scheduler *priority = priority_new();
        SchedulerMetrics metrics_prio = simulation_run(workload_priority, NUM_PROCESSES, priority);
        sc_priority->metrics_per_seed[s] = metrics_prio; // Salva para o JSON
        update_incremental_avg(&avg_priority, metrics_prio, k);
        priority_destroy(priority);
        destroy_workload(workload_priority);

        // Personalizado
        Process **workload_custom = generate_workload(current_seed, type);
        Scheduler *custom = custom_scheduler_new(workload_custom, NUM_PROCESSES);
        SchedulerMetrics metrics_custom = simulation_run(workload_custom, NUM_PROCESSES, custom);
        sc_custom->metrics_per_seed[s] = metrics_custom; // Salva para o JSON
        update_incremental_avg(&avg_custom, metrics_custom, k);
        custom_scheduler_destroy(custom);
        destroy_workload(workload_custom);
    }

    // Mantém o print das médias no console
    metrics_print(avg_fcfs, "FCFS");
    metrics_print(avg_round_robin, "Round Robin");
    metrics_print(avg_priority, "Prioridade");
    metrics_print(avg_custom, "Personalizado");
}

int main(void) {
    // Arrays que irão agrupar os resultados dos 4 cenários para cada algoritmo
    ScenarioData scenarios_fcfs[4];
    ScenarioData scenarios_rr[4];
    ScenarioData scenarios_priority[4];
    ScenarioData scenarios_custom[4];

    // Os IDs dos cenários (3º argumento) DEVEM bater com o dicionário do script Python
    run_scenario(SCENARIO_BALANCED, "Aleatório Equilibrado", "cenario_1_aleatorio_equilibrado", 
                 &scenarios_fcfs[0], &scenarios_rr[0], &scenarios_priority[0], &scenarios_custom[0]);

    run_scenario(SCENARIO_IO_BOUND, "I/O-Bound", "cenario_2_io_bound", 
                 &scenarios_fcfs[1], &scenarios_rr[1], &scenarios_priority[1], &scenarios_custom[1]);

    run_scenario(SCENARIO_CPU_BOUND, "CPU-Bound", "cenario_3_cpu_bound", 
                 &scenarios_fcfs[2], &scenarios_rr[2], &scenarios_priority[2], &scenarios_custom[2]);

    run_scenario(SCENARIO_UNBALANCED_PRIORITY, "Prioridades Desbalanceadas", "cenario_4_prioridades_desbalanceadas", 
                 &scenarios_fcfs[3], &scenarios_rr[3], &scenarios_priority[3], &scenarios_custom[3]);


    printf("\nSimulações concluídas. Exportando resultados para a pasta data/...\n");

    // Certifique-se de que a pasta 'data' exista ou o fopen poderá falhar!
    export_algorithm_metrics_json("data/results-fcfs.json", "FCFS", scenarios_fcfs, 4);
    export_algorithm_metrics_json("data/results-round-robin.json", "Round Robin", scenarios_rr, 4);
    export_algorithm_metrics_json("data/results-priority.json", "Priority", scenarios_priority, 4);
    export_algorithm_metrics_json("data/results-custom.json", "Personalizado", scenarios_custom, 4);

    // Limpeza da memória alocada para armazenar os dados de cada seed
    for (int i = 0; i < 4; i++) {
        free(scenarios_fcfs[i].metrics_per_seed);
        free(scenarios_rr[i].metrics_per_seed);
        free(scenarios_priority[i].metrics_per_seed);
        free(scenarios_custom[i].metrics_per_seed);
    }

    printf("Exportação concluída com sucesso.\n");
    return 0;
}