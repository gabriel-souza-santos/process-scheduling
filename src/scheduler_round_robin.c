#include "scheduler_round_robin.h"

#include "process.h"
#include "process_queue.h"
#include "scheduler.h"

size_t enqueue(Scheduler *s, ProcessControlBlock *ctx) {
    // Round Robin insere no final da fila (FIFO)
    return queue_insert(s->queue, ctx->process);
}

Process *dispatch(Scheduler *s, ProcessControlBlock *ctx) {
    return queue_remove(s->queue);
}

// O construtor agora recebe o tamanho do quantum desejado
Scheduler round_robin_new(Duration quantum) {
    Scheduler s;
    s.queue    = queue_new(NULL); // Não precisa ordenar, é por ordem de chegada
    s.quantum  = quantum;             // Quantum finito (força a preempção no simulador)
    s.enqueue  = enqueue;
    s.dispatch = dispatch;
    return s;
}

void round_robin_destroy(Scheduler *s) { 
    queue_destroy(&s->queue);
}