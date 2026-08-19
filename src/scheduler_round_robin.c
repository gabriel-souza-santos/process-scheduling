#include <stdlib.h>
#include "scheduler_round_robin.h"
#include "process.h"
#include "process_queue.h"
#include "scheduler.h"

static size_t enqueue(Scheduler *s, ProcessControlBlock *pcb, Duration current_time) {
    (void)current_time;
    // Round Robin insere no final da fila (FIFO)
    return queue_insert(s->queue, pcb->process);
}

static Process *dispatch(Scheduler *s, ProcessControlBlock pcb_table[], size_t size, Duration current_time) {
    (void)pcb_table;
    (void)size;
    (void)current_time;
    return queue_remove(s->queue);
}

// O construtor agora recebe o tamanho do quantum desejado
Scheduler *round_robin_new(Duration quantum) {
    Scheduler *s = malloc(sizeof(Scheduler));
    if (!s) return NULL;

    s->queue = queue_new(NULL); // Não precisa ordenar, é por ordem de chegada
    s->quantum = quantum;             // Quantum finito (força a preempção no simulador)
    s->enqueue = enqueue;
    s->dispatch = dispatch;
    return s;
}

void round_robin_destroy(Scheduler *s) { 
    queue_destroy(&s->queue);
}