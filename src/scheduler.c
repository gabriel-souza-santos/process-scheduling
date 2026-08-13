#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "process.h"
#include "process_queue.h"

/* -----------------------------------------------------------------------
 * Estrutura interna de rastreamento por processo durante a simulação
 * ----------------------------------------------------------------------- */

typedef struct {
    Duration total_cpu_used;   /* tempo total de CPU consumido */
    Duration first_run_time;   /* instante da primeira execução na CPU */
    bool first_run_recorded;   /* flag: já registrou a primeira execução? */
    bool completed;            /* processo já terminou? */
} ProcessTracking;

/* -----------------------------------------------------------------------
 * Utilitários internos
 * ----------------------------------------------------------------------- */

static SchedulerResult result_alloc(size_t count) {
    SchedulerResult r = {0};
    r.metrics = calloc(count, sizeof(ProcessMetrics));
    r.count = count;
    return r;
}

/* Preenche as médias globais do resultado após todas as métricas individuais
 * já terem sido calculadas. */
static void result_compute_averages(SchedulerResult *r, Duration cpu_busy_time) {
    double sum_tat = 0, sum_wt = 0, sum_rt = 0;

    for (size_t i = 0; i < r->count; i++) {
        sum_tat += r->metrics[i].turnaround_time;
        sum_wt += r->metrics[i].waiting_time;
        sum_rt += r->metrics[i].response_time;
    }

    r->avg_turnaround = sum_tat / (double)r->count;
    r->avg_waiting = sum_wt / (double)r->count;
    r->avg_response = sum_rt / (double)r->count;
    r->cpu_utilization = (r->total_time > 0)
                       ? (double)cpu_busy_time / (double)r->total_time
                       : 0.0;
}

/* Comparador de prioridade para queue_remove: retorna negativo se p1 tem
 * maior prioridade que p2 (ordem decrescente de priority). Em empate,
 * usa o arrival_time como desempate (FCFS). */
/* Localiza o índice de um processo pelo seu ID dentro do array `sorted` */
static size_t find_process_index(Process **sorted, size_t count, int pid) {
    for (size_t i = 0; i < count; i++) {
        if (process_id(sorted[i]) == pid) return i;
    }
    return count; /* não encontrado — não deve ocorrer */
}

static int cmp_priority(const Process *p1, const Process *p2) {
    int diff = process_priority((Process *)p2) - process_priority((Process *)p1);
    if (diff != 0) return diff;
    /* Desempate por tempo de chegada */
    Duration a1 = process_arrival_time((Process *)p1);
    Duration a2 = process_arrival_time((Process *)p2);
    if (a1 < a2) return -1;
    if (a1 > a2) return  1;
    return 0;
}

/* -----------------------------------------------------------------------
 * API pública de resultado
 * ----------------------------------------------------------------------- */

void scheduler_result_destroy(SchedulerResult *r) {
    if (r) {
        free(r->metrics);
        r->metrics = NULL;
        r->count = 0;
    }
}

void scheduler_result_print(const SchedulerResult *r, const char *algorithm_name) {
    if (!r || !r->metrics) return;

    printf("\n=== Algoritmo: %s ===\n", algorithm_name);
    printf("%-6s %-10s %-12s %-12s %-12s %-12s\n",
           "PID", "Chegada", "Conclusão", "Turnaround", "Espera", "Resposta");
    printf("%-6s %-10s %-12s %-12s %-12s %-12s\n",
           "---", "-------", "---------", "----------", "------", "--------");

    for (size_t i = 0; i < r->count; i++) {
        const ProcessMetrics *m = &r->metrics[i];
        printf("%-6d %-10zu %-12zu %-12zu %-12zu %-12zu\n",
               m->process_id,
               m->arrival_time,
               m->completion_time,
               m->turnaround_time,
               m->waiting_time,
               m->response_time);
    }

    printf("\nMédia Turnaround : %.2f\n", r->avg_turnaround);
    printf("Média Espera     : %.2f\n", r->avg_waiting);
    printf("Média Resposta   : %.2f\n", r->avg_response);
    printf("Utilização CPU   : %.1f%%\n", r->cpu_utilization * 100.0);
    printf("Tempo total      : %zu\n\n", r->total_time);
}

/* -----------------------------------------------------------------------
 * Comparador de arrival_time para ordenar processos antes de simular
 * ----------------------------------------------------------------------- */
static int cmp_arrival(const void *a, const void *b) {
    Process *pa = *(Process **)a;
    Process *pb = *(Process **)b;
    Duration ta = process_arrival_time(pa);
    Duration tb = process_arrival_time(pb);
    if (ta < tb) return -1;
    if (ta > tb) return  1;
    return 0;
}

/* -----------------------------------------------------------------------
 * FCFS — First Come, First Served
 * ----------------------------------------------------------------------- */
SchedulerResult scheduler_fcfs(Process **processes, size_t count) {
    SchedulerResult result = result_alloc(count);
    if (!result.metrics) return result;

    /* Copia e ordena por arrival_time para processar em ordem de chegada */
    Process **sorted = malloc(count * sizeof(Process *));
    if (!sorted) { scheduler_result_destroy(&result); return result; }
    memcpy(sorted, processes, count * sizeof(Process *));
    qsort(sorted, count, sizeof(Process *), cmp_arrival);

    ProcessTracking *track = calloc(count, sizeof(ProcessTracking));
    if (!track) {
        free(sorted);
        scheduler_result_destroy(&result);
        return result;
    }

    /* Mapeia process_id → índice em sorted/track */
    ProcessQueue *ready = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = NULL
    });

    /* Fila auxiliar para processos em E/S */
    ProcessQueue *io_queue = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = NULL
    });

    Duration clock = 0;
    Duration cpu_busy = 0;
    size_t arrived = 0;       /* quantos processos já entraram no sistema */
    size_t completed = 0;

    while (completed < count) {
        /* Admite processos que chegaram até este instante */
        while (arrived < count &&
               process_arrival_time(sorted[arrived]) <= clock) {
            queue_insert(ready, sorted[arrived]);
            arrived++;
        }

        /* Avança E/S dos processos em espera */
        /* (FCFS trata E/S de forma simples: avança 1 unidade por ciclo) */

        if (queue_size(ready) == 0) {
            /* CPU ociosa: avança até a chegada do próximo processo */
            if (arrived < count) {
                clock = process_arrival_time(sorted[arrived]);
            } else {
                /* Processa E/S enquanto nenhum processo está pronto */
                size_t io_size = queue_size(io_queue);
                for (size_t i = 0; i < io_size; i++) {
                    Process *io_p = queue_remove(io_queue);
                    process_wait(io_p, 1);
                    if (process_state(io_p) == PROCESS_READY) {
                        queue_insert(ready, io_p);
                    } else if (process_state(io_p) == PROCESS_WAITING) {
                        queue_insert(io_queue, io_p);
                    }
                }
                clock++;
            }
            continue;
        }

        /* Seleciona o próximo processo (frente da fila FIFO) */
        Process *current = queue_remove(ready);
        int cur_id = process_id(current);
        size_t cur_idx = find_process_index(sorted, count, cur_id);

        /* Registra primeira execução */
        if (!track[cur_idx].first_run_recorded) {
            track[cur_idx].first_run_time = clock;
            track[cur_idx].first_run_recorded = true;
        }

        /* Executa sem preempção até a rajada de CPU acabar */
        while (process_state(current) == PROCESS_RUNNING ||
               process_state(current) == PROCESS_READY) {
            Duration consumed = process_execute(current, (Duration)(-1)); /* sem limite */
            clock += consumed;
            cpu_busy += consumed;
            track[cur_idx].total_cpu_used += consumed;

            /* Admite processos que chegaram durante esta execução */
            while (arrived < count &&
                   process_arrival_time(sorted[arrived]) <= clock) {
                queue_insert(ready, sorted[arrived]);
                arrived++;
            }

            /* Avança E/S em paralelo (simples: avança pelo tempo executado) */
            size_t io_size = queue_size(io_queue);
            for (size_t i = 0; i < io_size; i++) {
                Process *io_p = queue_remove(io_queue);
                process_wait(io_p, consumed);
                if (process_state(io_p) == PROCESS_READY) {
                    queue_insert(ready, io_p);
                } else if (process_state(io_p) == PROCESS_WAITING) {
                    queue_insert(io_queue, io_p);
                }
            }

            if (process_state(current) == PROCESS_TERMINATED) break;
        }

        if (process_state(current) == PROCESS_WAITING) {
            /* Processo foi para E/S */
            queue_insert(io_queue, current);
        } else if (process_state(current) == PROCESS_TERMINATED) {
            track[cur_idx].completed = true;
            result.metrics[cur_idx] = (ProcessMetrics){
                .process_id      = cur_id,
                .arrival_time    = process_arrival_time(current),
                .completion_time = clock,
                .turnaround_time = clock - process_arrival_time(current),
                .waiting_time    = clock - process_arrival_time(current)
                                   - track[cur_idx].total_cpu_used,
                .response_time   = track[cur_idx].first_run_time
                                   - process_arrival_time(current),
            };
            completed++;
        }
    }

    result.total_time = clock;
    result_compute_averages(&result, cpu_busy);

    queue_destroy(&ready);
    queue_destroy(&io_queue);
    free(track);
    free(sorted);

    return result;
}

/* -----------------------------------------------------------------------
 * Round Robin
 * ----------------------------------------------------------------------- */
SchedulerResult scheduler_round_robin(Process **processes, size_t count,
                                      Duration quantum) {
    SchedulerResult result = result_alloc(count);
    if (!result.metrics) return result;

    Process **sorted = malloc(count * sizeof(Process *));
    if (!sorted) { scheduler_result_destroy(&result); return result; }
    memcpy(sorted, processes, count * sizeof(Process *));
    qsort(sorted, count, sizeof(Process *), cmp_arrival);

    ProcessTracking *track = calloc(count, sizeof(ProcessTracking));
    if (!track) {
        free(sorted);
        scheduler_result_destroy(&result);
        return result;
    }

    ProcessQueue *ready = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = NULL /* FIFO */
    });

    ProcessQueue *io_queue = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = NULL
    });

    Duration clock = 0;
    Duration cpu_busy = 0;
    size_t arrived = 0;
    size_t completed = 0;

    /* Admite processos que chegam no instante 0 */
    while (arrived < count &&
           process_arrival_time(sorted[arrived]) <= clock) {
        queue_insert(ready, sorted[arrived]);
        arrived++;
    }

    while (completed < count) {
        if (queue_size(ready) == 0) {
            /* CPU ociosa: avança 1 unidade e processa E/S */
            size_t io_size = queue_size(io_queue);
            for (size_t i = 0; i < io_size; i++) {
                Process *io_p = queue_remove(io_queue);
                process_wait(io_p, 1);
                if (process_state(io_p) == PROCESS_READY) {
                    queue_insert(ready, io_p);
                } else {
                    queue_insert(io_queue, io_p);
                }
            }
            clock++;

            while (arrived < count &&
                   process_arrival_time(sorted[arrived]) <= clock) {
                queue_insert(ready, sorted[arrived]);
                arrived++;
            }
            continue;
        }

        /* Seleciona o processo na frente da fila */
        Process *current = queue_remove(ready);
        int cur_id = process_id(current);
        size_t cur_idx = find_process_index(sorted, count, cur_id);

        /* Registra primeira execução */
        if (!track[cur_idx].first_run_recorded) {
            track[cur_idx].first_run_time = clock;
            track[cur_idx].first_run_recorded = true;
        }

        /* Executa por no máximo `quantum` unidades */
        Duration consumed = process_execute(current, quantum);
        clock += consumed;
        cpu_busy += consumed;
        track[cur_idx].total_cpu_used += consumed;

        /* Admite chegadas durante a fatia */
        while (arrived < count &&
               process_arrival_time(sorted[arrived]) <= clock) {
            queue_insert(ready, sorted[arrived]);
            arrived++;
        }

        /* Avança E/S em paralelo pelo tempo executado */
        size_t io_size = queue_size(io_queue);
        for (size_t i = 0; i < io_size; i++) {
            Process *io_p = queue_remove(io_queue);
            process_wait(io_p, consumed);
            if (process_state(io_p) == PROCESS_READY) {
                queue_insert(ready, io_p);
            } else {
                queue_insert(io_queue, io_p);
            }
        }

        ProcessState st = process_state(current);

        if (st == PROCESS_TERMINATED) {
            track[cur_idx].completed = true;
            result.metrics[cur_idx] = (ProcessMetrics){
                .process_id      = cur_id,
                .arrival_time    = process_arrival_time(current),
                .completion_time = clock,
                .turnaround_time = clock - process_arrival_time(current),
                .waiting_time    = clock - process_arrival_time(current)
                                   - track[cur_idx].total_cpu_used,
                .response_time   = track[cur_idx].first_run_time
                                   - process_arrival_time(current),
            };
            completed++;
        } else if (st == PROCESS_WAITING) {
            /* Foi para E/S: entra na fila de E/S */
            queue_insert(io_queue, current);
        } else {
            /* READY: quantum esgotado, vai para o final da fila de prontos */
            queue_insert(ready, current);
        }
    }

    result.total_time = clock;
    result_compute_averages(&result, cpu_busy);

    queue_destroy(&ready);
    queue_destroy(&io_queue);
    free(track);
    free(sorted);

    return result;
}

/* -----------------------------------------------------------------------
 * Escalonamento por Prioridade Não Preemptivo
 * ----------------------------------------------------------------------- */
SchedulerResult scheduler_priority(Process **processes, size_t count) {
    SchedulerResult result = result_alloc(count);
    if (!result.metrics) return result;

    Process **sorted = malloc(count * sizeof(Process *));
    if (!sorted) { scheduler_result_destroy(&result); return result; }
    memcpy(sorted, processes, count * sizeof(Process *));
    qsort(sorted, count, sizeof(Process *), cmp_arrival);

    ProcessTracking *track = calloc(count, sizeof(ProcessTracking));
    if (!track) {
        free(sorted);
        scheduler_result_destroy(&result);
        return result;
    }

    /* Fila com comparador de prioridade: queue_remove devolve o de maior
     * prioridade presente na fila no momento da chamada */
    ProcessQueue *ready = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = cmp_priority
    });

    ProcessQueue *io_queue = queue_new((QueueAttr){
        .capacity = count,
        .cmp_func = NULL
    });

    Duration clock = 0;
    Duration cpu_busy = 0;
    size_t arrived = 0;
    size_t completed = 0;

    while (arrived < count &&
           process_arrival_time(sorted[arrived]) <= clock) {
        queue_insert(ready, sorted[arrived]);
        arrived++;
    }

    while (completed < count) {
        if (queue_size(ready) == 0) {
            /* CPU ociosa */
            size_t io_size = queue_size(io_queue);
            for (size_t i = 0; i < io_size; i++) {
                Process *io_p = queue_remove(io_queue);
                process_wait(io_p, 1);
                if (process_state(io_p) == PROCESS_READY) {
                    queue_insert(ready, io_p);
                } else {
                    queue_insert(io_queue, io_p);
                }
            }
            clock++;

            while (arrived < count &&
                   process_arrival_time(sorted[arrived]) <= clock) {
                queue_insert(ready, sorted[arrived]);
                arrived++;
            }
            continue;
        }

        /* Seleciona o processo de maior prioridade disponível */
        Process *current = queue_remove(ready);
        int cur_id = process_id(current);
        size_t cur_idx = find_process_index(sorted, count, cur_id);

        if (!track[cur_idx].first_run_recorded) {
            track[cur_idx].first_run_time = clock;
            track[cur_idx].first_run_recorded = true;
        }

        /* Não preemptivo: executa até a rajada de CPU acabar */
        while (process_state(current) == PROCESS_RUNNING ||
               process_state(current) == PROCESS_READY) {
            Duration consumed = process_execute(current, (Duration)(-1));
            clock += consumed;
            cpu_busy += consumed;
            track[cur_idx].total_cpu_used += consumed;

            while (arrived < count &&
                   process_arrival_time(sorted[arrived]) <= clock) {
                queue_insert(ready, sorted[arrived]);
                arrived++;
            }

            /* Avança E/S em paralelo */
            size_t io_size = queue_size(io_queue);
            for (size_t i = 0; i < io_size; i++) {
                Process *io_p = queue_remove(io_queue);
                process_wait(io_p, consumed);
                if (process_state(io_p) == PROCESS_READY) {
                    queue_insert(ready, io_p);
                } else {
                    queue_insert(io_queue, io_p);
                }
            }

            if (process_state(current) == PROCESS_TERMINATED) break;
        }

        if (process_state(current) == PROCESS_WAITING) {
            queue_insert(io_queue, current);
        } else if (process_state(current) == PROCESS_TERMINATED) {
            track[cur_idx].completed = true;
            result.metrics[cur_idx] = (ProcessMetrics){
                .process_id      = cur_id,
                .arrival_time    = process_arrival_time(current),
                .completion_time = clock,
                .turnaround_time = clock - process_arrival_time(current),
                .waiting_time    = clock - process_arrival_time(current)
                                   - track[cur_idx].total_cpu_used,
                .response_time   = track[cur_idx].first_run_time
                                   - process_arrival_time(current),
            };
            completed++;
        }
    }

    result.total_time = clock;
    result_compute_averages(&result, cpu_busy);

    queue_destroy(&ready);
    queue_destroy(&io_queue);
    free(track);
    free(sorted);

    return result;
}

/* -----------------------------------------------------------------------
 * Funções auxiliares (assinaturas em scheduler.h)
 * ----------------------------------------------------------------------- */

/* TODO: implementar busca do processo com menor tempo de chegada */
size_t scheduler_earliest_arrival(Process **processes, size_t count) {
    (void)processes;
    (void)count;
    return 0;
}

/* TODO: implementar média ponderada por prioridade */
double scheduler_weighted_avg_waiting(const SchedulerResult *r,
                                      Process **processes, size_t count) {
    (void)r;
    (void)processes;
    (void)count;
    return 0.0;
}

/* TODO: implementar geração do diagrama de Gantt em string */
char *scheduler_gantt_chart(const SchedulerResult *r) {
    (void)r;
    return NULL;
}
