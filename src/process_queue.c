#include <stdlib.h>
#include "process_queue.h"

struct ProcessQueue {
    size_t capacity;
    size_t size;
    size_t front;
    size_t rear;
    Process **items;
    ProcessComparator cmp_func;
};

ProcessQueue *queue_new(QueueAttr attr) {
    ProcessQueue *q = malloc(sizeof(ProcessQueue));
    if (!q) return NULL;

    q->items = calloc(attr.capacity, sizeof(Process *));
    if (!q->items) {
        free(q);
        return NULL;
    }

    q->capacity = attr.capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    q->cmp_func = attr.cmp_func;

    return q;
}

void queue_destroy(ProcessQueue **q) {
    if (q && *q) {
        free((*q)->items);
        free(*q);
        *q = NULL;
    }
}

size_t queue_size(ProcessQueue *q) {
    return q->size;
}



Process *queue_at(ProcessQueue *q, size_t index) {
    if (index >= q->size) {
        return NULL; /* Índice fora do intervalo */
    }
    size_t actual_index = (q->front + index) % q->capacity;
    return q->items[actual_index];
}

void queue_insert(ProcessQueue *restrict q, Process *restrict p) {
    if (q->size == q->capacity) {
         return; /* Fila cheia, não é possível inserir */
    }

    q->items[q->rear] = p;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
}

Process *queue_remove(ProcessQueue *q) {
    if (q->size == 0) {
        return NULL; /* Fila vazia, não é possível remover */
    }

    size_t target = q->front;

    if (q->cmp_func) {
        /* Busca o elemento de maior prioridade na janela [front, rear) */
        target = q->front;
        for (size_t i = 1; i < q->size; i++) {
            size_t idx = (q->front + i) % q->capacity;
            if (q->cmp_func(q->items[idx], q->items[target]) < 0) {
                target = idx;
            }
        }
    }

    Process *p = q->items[target];

    /* Fecha o buraco deslocando os elementos entre front e target */
    size_t index = target;
    while (index != q->front) {
        size_t prev = (index == 0) ? q->capacity - 1 : index - 1;
        q->items[index] = q->items[prev];
        index = prev;
    }

    q->items[q->front] = NULL;
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    return p;
}