#include "scheduler_fcfs.h"
#include "process.h"
#include "process_queue.h"
#include "scheduler.h"
#include <stddef.h>

size_t enqueue(Scheduler *s, ProcessControlBlock *ctx) {
    // FCFS não utiliza o quantum, apenas adiciona no final da fila
    return queue_insert(s->queue, ctx->process);
}

Process *dispatch(Scheduler *s, ProcessControlBlock *ctx) {
    return queue_remove(s->queue);
}

Scheduler fcfs_new(void) {
    Scheduler s;
    s.queue    = queue_new(NULL); // FCFS não precisa de função de comparação
    s.quantum  = DURATION_INF; // Roda até o final da rajada de CPU
    s.enqueue  = enqueue;
    s.dispatch = dispatch;
    return s;
}

void fcfs_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}