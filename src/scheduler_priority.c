#include "process.h"
#include "process_queue.h"
#include "scheduler.h"
#include <stdlib.h>

// Retorna < 0 se 'a' deve vir antes de 'b'
static int compare_processes(const Process *p1, const Process *p2) {
    return process_priority(p2) - process_priority(p1); 
}

static size_t enqueue(Scheduler *s, ProcessControlBlock *pcb, Duration current_time) {
    (void)current_time;
    // A fila usará a priority_cmp para inserir o processo no lugar certo
    return queue_insert(s->queue, pcb->process);
}

static Process *dispatch(Scheduler *s, ProcessControlBlock pcb_table[], size_t size, Duration current_time) {
    // Remove o processo do início (graças à ordenação, será o de maior prioridade)
    (void)pcb_table;
    (void)size;
    (void)current_time;
    return queue_remove(s->queue);
}

Scheduler *priority_new(void) {
    Scheduler *s = malloc(sizeof(Scheduler));
    if (!s) return NULL;

    s->queue = queue_new(compare_processes); // Fila ordenada por prioridade
    s->quantum = DURATION_INF; // Não preemptivo: roda até acabar ou pedir E/S
    s->enqueue = enqueue;
    s->dispatch = dispatch;
    return s;
}

void priority_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}