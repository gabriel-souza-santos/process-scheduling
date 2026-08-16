#include "scheduler_round_robin.h"

#include "process.h"
#include "process_queue.h"
#include "scheduler.h"
// #include "queue.h" (assumindo que seja onde a fila está definida)

size_t rr_enqueue(ProcessQueue *q, Process *p) {
    // Round Robin insere no final da fila (FIFO)
    return queue_insert(q, p);
}

Process *rr_dispatch(ProcessQueue *q) {
    return queue_remove(q);
}

// O construtor agora recebe o tamanho do quantum desejado
Scheduler round_robin_new(Duration quantum) {
    Scheduler s;
    s.queue = queue_new(NULL); // Não precisa ordenar, é por ordem de chegada
    s.quantum = quantum; // Quantum finito (força a preempção no simulador)
    s.enqueue = rr_enqueue;
    s.dispatch = rr_dispatch;
    return s;
}

void round_robin_destroy(Scheduler *s) { 
    queue_destroy(&s->queue);
}