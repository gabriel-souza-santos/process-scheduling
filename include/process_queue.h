# ifndef PROCESS_QUEUE_H
# define PROCESS_QUEUE_H

#include <stddef.h>
#include "process.h"

typedef struct ProcessQueue ProcessQueue;

typedef int (*ProcessComparator)(const Process *p1, const Process *p2);

typedef struct {
    size_t capacity; /* capacidade máxima da fila */
    ProcessComparator cmp_func; /* função de comparação para busca na fila, NULL para FIFO */
} QueueAttr;

ProcessQueue *queue_new(QueueAttr attr);
void queue_destroy(ProcessQueue **q);

void queue_insert(ProcessQueue *q, Process *p);
Process *queue_remove(ProcessQueue *q);
bool queue_is_empty(ProcessQueue *q);

#endif /* PROCESS_QUEUE_H */