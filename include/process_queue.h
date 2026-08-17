#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include <stddef.h>
#include "process.h"

typedef struct ProcessQueue ProcessQueue;

typedef int (*ProcessComparator)(const Process *p1, const Process *p2);
typedef int (*QueueIterator)(Process *p, void *args);

ProcessQueue *queue_new(ProcessComparator cmp);
void queue_destroy(ProcessQueue **q);

size_t queue_size(ProcessQueue *q);
Process *queue_at(ProcessQueue *q, size_t index); /* Retorna o processo na posição index da fila sem removê-lo */

Process *queue_peek(ProcessQueue *q); /* Retorna o processo no início da fila sem removê-lo */
size_t queue_insert(ProcessQueue *restrict q, Process *restrict p);
Process *queue_remove(ProcessQueue *q);

Process *queue_for_each(ProcessQueue *q, QueueIterator iter);

#endif /* PROCESS_QUEUE_H */