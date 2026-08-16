#include "process.h"
#include "scheduler.h"
#include <stdlib.h>

#define CONTEXT_SWITCH_COST 2000 // Custo de troca de contexto (2.000 ns / 2 microssegundos)
#define NS_PER_ITERATION    5    // Cada nó percorrido na fila custa 5 ns

typedef struct {
    Process *process;     // O processo em si
    Duration wake_time;   // Quando deve acordar de E/S (DURATION_INF se não estiver em E/S)
    Duration finish_time; // Instante em que concluiu toda a execução
    Duration cpu_time;    // Tempo total de CPU consumido 
} ProcessControlBlock;

SchedulerMetrics simulation_run(Process **processes, size_t num_processes, Scheduler scheduler) {
    SchedulerMetrics metrics = {0};
    Duration current_time = 0;
    Process *running = NULL;
    size_t context_switches = 0;
    size_t current_index = 0;
    size_t finished = 0;

    ProcessControlBlock *control_blocks = malloc(num_processes * sizeof(ProcessControlBlock));
    if (!control_blocks) return metrics; // Falha na alocação, retorna métricas vazias

    for (size_t i = 0; i < num_processes; i++) {
        control_blocks[i].process = processes[i];
        control_blocks[i].wake_time = DURATION_INF;
        control_blocks[i].finish_time = 0;
        control_blocks[i].cpu_time = 0;
    }

    while (finished < num_processes) {

        /* PROCESSAR NOVAS CHEGADAS */
        while (current_index < num_processes &&
               process_arrival_time(processes[current_index]) <= current_time) {

            size_t steps = scheduler.enqueue(scheduler.queue, processes[current_index]);
            current_time += (Duration)(steps * NS_PER_ITERATION);
            current_index++;
        }

        /* PROCESSAR RETORNOS DE E/S */
        for (size_t i = 0; i < num_processes; i++) {
            if (control_blocks[i].wake_time <= current_time) {
                control_blocks[i].wake_time = DURATION_INF; // Desliga o alarme
                size_t steps = scheduler.enqueue(scheduler.queue, control_blocks[i].process);
                current_time += (Duration)(steps * NS_PER_ITERATION);
            }
        }

        /* DESPACHO DA CPU (Se estiver livre, pede ao escalonador) */
        if (!running) {
            running = scheduler.dispatch(scheduler.queue);
            if (running) {
                context_switches++; // Contabiliza a troca de contexto
                current_time += CONTEXT_SWITCH_COST; // Adiciona o custo de troca de contexto
            }
        }

        /* EXECUÇÃO NA CPU OU AVANÇO NO TEMPO (IDLE) */
        if (running) {
            size_t pid = (size_t)process_id(running);

            // Executa o processo na CPU 
            Duration time_consumed = process_execute(running, scheduler.quantum);
            current_time += time_consumed;
            control_blocks[pid].cpu_time += time_consumed;

            ProcessState state = process_state(running);

            if (state == PROCESS_TERMINATED) {
                // Caso A: O processo terminou a execução
                control_blocks[pid].finish_time = current_time;
                finished++;
                running = NULL; // Libera a CPU
            } 
            else if (state == PROCESS_WAITING) {
                // Caso B: O processo bloqueou pedindo E/S
                Duration io_duration = process_wait(running, DURATION_INF);
                
                // Configura a hora em que o processo deve acordar no PCB
                control_blocks[pid].wake_time = current_time + io_duration;
                running = NULL; // Libera a CPU
            }
        } 
        else {
            /* SALTO NO TEMPO (CPU Ociosa)
             * Se a CPU não rodou ninguém, avançamos o relógio para o próximo
             * evento futuro (seja uma nova chegada ou alguém acordando de E/S). */
            Duration next_event = DURATION_INF;

            // Próxima chegada?
            if (current_index < num_processes) {
                next_event = process_arrival_time(processes[current_index]);
            }

            // Próximo retorno de E/S mais próximo?
            for (size_t i = 0; i < num_processes; i++) {
                if (control_blocks[i].wake_time < next_event) {
                    next_event = control_blocks[i].wake_time;
                }
            }

            if (next_event != DURATION_INF) {
                current_time = next_event; // Salta direto para o evento!
            }
        }
    }


    double mean_turnaround = 0.0;
    double mean_slowdown = 0.0;
    double m2_slowdown = 0.0;  // soma dos quadrados das diferenças (Welford)
    double jain_index = 0.0;

    for (size_t i = 0; i < num_processes; i++) {
        Process *p = control_blocks[i].process;

        Duration arrival = process_arrival_time(p);
        Duration finish = control_blocks[i].finish_time;

        Duration turnaround = finish - arrival;
        Duration cpu_time = control_blocks[i].cpu_time;
        double slowdown = (double)turnaround / (double)cpu_time;

        size_t n = i + 1;

        // Atualização incremental da média de turnaround
        double delta_t = (double)turnaround - mean_turnaround;
        mean_turnaround += delta_t / n;

        // Atualização incremental da média e M2 de slowdown (Welford)
        double delta_s = slowdown - mean_slowdown;
        mean_slowdown += delta_s / n;

        double delta_s2 = slowdown - mean_slowdown;
        m2_slowdown += delta_s * delta_s2;
    }

    // Reconstrói sum_slowdown e sum_slowdown_sq a partir da média e do M2
    double sum_slowdown = mean_slowdown * num_processes;
    double sum_slowdown_sq = m2_slowdown + num_processes * mean_slowdown * mean_slowdown;

    if (sum_slowdown_sq > 0) {
        jain_index = (sum_slowdown * sum_slowdown) / (num_processes * sum_slowdown_sq);
    } else {
        jain_index = 1.0;
    }

    metrics.average_turnaround = mean_turnaround;
    metrics.average_slowdown = mean_slowdown;
    metrics.context_switches = context_switches;
    metrics.jain_index = jain_index;
    metrics.total_time = current_time;

    free(control_blocks);

    return metrics;
}