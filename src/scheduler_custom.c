#include <stddef.h>
#include <stdlib.h>
#include "scheduler_custom.h"

#define MIN_QUANTUM      1000
#define MAX_BURST_BONUS  5      // teto pro bônus de rajada — nunca domina a prioridade base
#define AGING_STEP       5000   // a cada 5000ns de espera sem rodar, +1 de prioridade

typedef struct {
    Duration last_seen; // último instante em que o processo saiu do estado "pronto" (foi despachado) ou chegou
} CustomProcessInfo;

typedef struct {
    Scheduler base;
    CustomProcessInfo *info;
} CustomScheduler;

static int compute_burst_bonus(Duration avg_burst) {
    if (avg_burst <= 0) return MAX_BURST_BONUS / 2;   // sem histórico ainda: bônus neutro
    if (avg_burst >= MIN_QUANTUM) return 0;            // processo claramente CPU-bound: sem bônus
    double ratio = 1.0 - ((double)avg_burst / (double)MIN_QUANTUM);
    return (int)(ratio * MAX_BURST_BONUS);
}

static int compute_aging_bonus(Duration since_last_run) {
    if (since_last_run <= 0) return 0;
    return (int)(since_last_run / AGING_STEP);          // sem teto: garante que starvation eventualmente é revertida
}

static int compare_processes(const Process *p1, const Process *p2) {
    return process_priority(p2) - process_priority(p1);
}

static size_t enqueue(Scheduler *s, ProcessControlBlock *pcb, Duration current_time) {
    CustomScheduler *cs = (CustomScheduler *)s;
    int pid = process_id(pcb->process);

    Duration since_last_run = current_time - cs->info[pid].last_seen;

    size_t burst_count = (pcb->burst_count != 0) ? pcb->burst_count : 1;
    Duration avg_burst = pcb->cpu_time / burst_count;

    int burst_bonus = compute_burst_bonus(avg_burst);
    int aging_bonus = compute_aging_bonus(since_last_run);
    int dynamic_priority = pcb->base_priority + burst_bonus + aging_bonus;

    process_set_priority(pcb->process, dynamic_priority);
    return queue_insert(cs->base.queue, pcb->process);
}

static Process *dispatch(Scheduler *s, ProcessControlBlock pcb_table[], size_t size, Duration current_time) {
    CustomScheduler *cs = (CustomScheduler *)s;
    Process *process = queue_peek(cs->base.queue);
    if (!process) return NULL;

    int pid = process_id(process);
    if (pid < 0 || pid >= (int)size) return NULL;

    cs->info[pid].last_seen = current_time; // "zera" o relógio de aging: o processo está prestes a rodar

    ProcessControlBlock pcb = pcb_table[pid];
    size_t burst_count = (pcb.burst_count != 0) ? pcb.burst_count : 1;
    Duration avg_burst = pcb.cpu_time / burst_count;

    cs->base.quantum = (avg_burst > MIN_QUANTUM) ? avg_burst : MIN_QUANTUM;
    return queue_remove(cs->base.queue);
}

Scheduler *custom_scheduler_new(Process *processes[], size_t size) {
    CustomScheduler *cs = malloc(sizeof(CustomScheduler));
    cs->base.queue = queue_new(compare_processes);
    cs->base.quantum = MIN_QUANTUM;
    cs->base.enqueue = enqueue;
    cs->base.dispatch = dispatch;
    cs->info = malloc(size * sizeof(CustomProcessInfo));

    for (size_t i = 0; i < size; i++) {
        cs->info[i].last_seen = process_arrival_time(processes[i]); // relógio de aging começa na chegada
    }
    return (Scheduler *)cs;
}

void custom_scheduler_destroy(Scheduler *s) {
    CustomScheduler *cs = (CustomScheduler *)s;
    queue_destroy(&cs->base.queue);
    free(cs->info);
    free(cs);
}