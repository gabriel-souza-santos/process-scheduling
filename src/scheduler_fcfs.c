#include "scheduler_fcfs.h"
#include "process.h"
#include "process_queue.h"
#include "scheduler.h"
#include <stddef.h>
#include <stdlib.h>

static size_t enqueue(Scheduler *s, ProcessControlBlock *pcb, Duration current_time) {
    (void)current_time;
    // FCFS não utiliza o quantum, apenas adiciona no final da fila
    return queue_insert(s->queue, pcb->process);
}

static Process *dispatch(Scheduler *s, ProcessControlBlock pcb_table[], size_t size, Duration current_time) {
    (void)pcb_table;
    (void)size;
    (void)current_time;
    return queue_remove(s->queue);
}

Scheduler *fcfs_new(void) {
    Scheduler *s = malloc(sizeof(Scheduler));
    if (!s) return NULL;

    s->queue = queue_new(NULL); // FCFS não precisa de função de comparação
    s->quantum = DURATION_INF; // Roda até o final da rajada de CPU
    s->enqueue = enqueue;
    s->dispatch = dispatch;
    return s;
}

void fcfs_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}