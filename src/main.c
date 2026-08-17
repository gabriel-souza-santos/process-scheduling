#include <stdio.h>
#include <stdlib.h>
#include "simulation.h"
#include "scheduler.h"
#include "scheduler_fcfs.h"
#include "scheduler_round_robin.h"
#include "scheduler_priority.h"
#include "seed_generator.h"

#define NUM_PROCESSES 1000
#define NUM_SEEDS 100
#define RESULTS_FILE "data/results.json"

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
            if (rng_between(1, 100) <= 80) {
                priority = rng_between(8, 10);
            } else {
                priority = rng_between(1, 3);
            }
        }
        
        size_t num_bursts;
        size_t cpu_min, cpu_max;
        size_t io_min, io_max;
        size_t arrival_min, arrival_max;

        switch (type) {
            case SCENARIO_IO_BOUND:
                num_bursts = rng_between(10, 20) * 2 - 1; 
                cpu_min = 500;  cpu_max = 1500; 
                io_min  = 3000; io_max = 8000;  
                arrival_min = 0; arrival_max = 6000000; 
                break;
            case SCENARIO_CPU_BOUND:
                num_bursts = rng_between(1, 3) * 2 - 1;
                cpu_min = 8000; cpu_max = 20000; 
                io_min  = 500;  io_max = 1000;
                arrival_min = 0; arrival_max = 11000000;
                break;
            case SCENARIO_UNBALANCED_PRIORITY:
            case SCENARIO_BALANCED:
            default:
                num_bursts = rng_between(3, 8) * 2 - 1;
                cpu_min = 2000; cpu_max = 8000; 
                io_min  = 2000; io_max = 8000;
                arrival_min = 0; arrival_max = 10000000;
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

void run_scenario(ScenarioType type, const char* scenario_name) {
    unsigned int base_seed = 12345;

    printf("\nExecutando Cenário: %s...\n", scenario_name);

    SchedulerMetrics avg_fcfs = {0};
    SchedulerMetrics avg_rr   = {0};
    SchedulerMetrics avg_prio = {0};

    for (int s = 0; s < NUM_SEEDS; s++) {
        unsigned int current_seed = base_seed + s;
        size_t k = s + 1; // Contador de amostras para a média incremental

        // FCFS
        Process **workload_fcfs = generate_workload(current_seed, type);
        Scheduler fcfs = fcfs_new();
        SchedulerMetrics metrics_fcfs = simulation_run(workload_fcfs, NUM_PROCESSES, fcfs);
        update_incremental_avg(&avg_fcfs, metrics_fcfs, k);
        fcfs_destroy(&fcfs);
        destroy_workload(workload_fcfs);
        
        // Round Robin
        Process **workload_rr = generate_workload(current_seed, type);
        Scheduler round_robin = round_robin_new(2000); 
        SchedulerMetrics metrics_rr = simulation_run(workload_rr, NUM_PROCESSES, round_robin);
        update_incremental_avg(&avg_rr, metrics_rr, k);
        round_robin_destroy(&round_robin);
        destroy_workload(workload_rr);
        
        // Priority
        Process **workload_priority = generate_workload(current_seed, type);
        Scheduler priority = priority_new();
        SchedulerMetrics metrics_prio = simulation_run(workload_priority, NUM_PROCESSES, priority);
        update_incremental_avg(&avg_prio, metrics_prio, k);
        priority_destroy(&priority);
        destroy_workload(workload_priority);
    }

    metrics_print(avg_fcfs, "FCFS");
    metrics_print(avg_rr, "Round Robin");
    metrics_print(avg_prio, "Prioridade");
}

int main(void) {

    run_scenario(SCENARIO_BALANCED, "Aleatório Equilibrado");
    run_scenario(SCENARIO_IO_BOUND, "I/O-Bound");
    run_scenario(SCENARIO_CPU_BOUND, "CPU-Bound");
    run_scenario(SCENARIO_UNBALANCED_PRIORITY, "Prioridades Desbalanceadas");

    printf("\nSimulação concluída");
    return 0;
}
