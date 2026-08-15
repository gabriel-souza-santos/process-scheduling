#include "process.h"
#include "process_queue.h"
#include "scheduler.h"

// Retorna < 0 se 'a' deve vir antes de 'b'
int priority_cmp(const Process *p1, const Process *p2) {
    return process_priority(p1) - process_priority(p2); 
}

void priority_enqueue(ProcessQueue *q, Process *p) {
    // A fila usará a priority_cmp para inserir o processo no lugar certo
    queue_insert(q, p);
}

Process *priority_dispatch(ProcessQueue *q) {
    // Remove o processo do início (graças à ordenação, será o de maior prioridade)
    return queue_remove(q);
}

Scheduler priority_new(void) {
    Scheduler s;
    s.queue = queue_new((QueueAttr){
        .cmp_func = priority_cmp,
        .capacity = 0,    
    });

    s.quantum = DURATION_INF; // Não preemptivo: roda até acabar ou pedir E/S
    s.enqueue = priority_enqueue;
    s.dispatch = priority_dispatch;
    return s;
}

void priority_destroy(Scheduler *s) {
    queue_destroy(&s->queue);
}