#include "scheduler_fcfs.h"
#include "process_queue.h"
#include <stddef.h>

size_t fcfs_enqueue(ProcessQueue *q, Process *p) {
    // FCFS não utiliza o quantum, apenas adiciona no final da fila
    return queue_insert(q, p);
}

Process *fcfs_dispatch(ProcessQueue *q) {
    return queue_remove(q);
}

Scheduler fcfs_new(void) {
    Scheduler s;
    s.queue = queue_new(NULL); // FCFS não precisa de função de comparação
    s.quantum = DURATION_INF; // Roda até o final da rajada de CPU
    s.enqueue = fcfs_enqueue;
    s.dispatch = fcfs_dispatch;
    return s;
}

void fcfs_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}