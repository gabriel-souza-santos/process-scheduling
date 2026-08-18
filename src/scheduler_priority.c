#include "process.h"
#include "process_queue.h"
#include "scheduler.h"

// Retorna < 0 se 'a' deve vir antes de 'b'
int compare_processes(const Process *p1, const Process *p2) {
    return process_priority(p1) - process_priority(p2); 
}

size_t enqueue(Scheduler *s, ProcessControlBlock *ctx) {
    // A fila usará a priority_cmp para inserir o processo no lugar certo
    return queue_insert(s->queue, ctx->process);
}

Process *dispatch(Scheduler *s, ProcessControlBlock *ctx) {
    // Remove o processo do início (graças à ordenação, será o de maior prioridade)
    return queue_remove(s->queue);
}

Scheduler priority_new(void) {
    Scheduler s;
    s.queue    = queue_new(compare_processes); // Fila ordenada por prioridade
    s.quantum  = DURATION_INF; // Não preemptivo: roda até acabar ou pedir E/S
    s.enqueue  = enqueue;
    s.dispatch = dispatch;
    return s;
}

void priority_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}