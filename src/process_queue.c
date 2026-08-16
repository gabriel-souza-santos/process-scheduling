#include <stdlib.h>
#include "process_queue.h"

typedef struct Node {
    Process *process;
    struct Node *next;
} Node;

struct ProcessQueue {
    size_t size;
    Node *head;
    Node *tail;
    ProcessComparator cmp;
};

ProcessQueue *queue_new(ProcessComparator cmp) {
    ProcessQueue *q = malloc(sizeof(ProcessQueue));
    if (!q) return NULL;

    q->size = 0;
    q->head = NULL;
    q->tail = NULL;
    q->cmp = cmp;

    return q;
}

void queue_destroy(ProcessQueue **q) {
    if (q && *q) {
        Node *current = (*q)->head;
        while (current) {
            Node *next = current->next;
            free(current);
            current = next;
        }
        free(*q);
        *q = NULL;
    }
}

size_t queue_size(ProcessQueue *q) {
    return q ? q->size : 0;
}

Process *queue_peek(ProcessQueue *q) {
    if (!q || !q->head) return NULL;
    return q->head->process;
}

Process *queue_at(ProcessQueue *q, size_t index) {
    if (!q || index >= q->size) {
        return NULL; /* Índice fora do intervalo */
    }

    Node *current = q->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }

    return current ? current->process : NULL;
}

size_t queue_insert(ProcessQueue *restrict q, Process *restrict p) {
    if (!q || !p) return 0;

    Node *new_node = malloc(sizeof(Node));
    if (!new_node) return 0;

    new_node->process = p;
    new_node->next = NULL;

    /* Caso 1: FCFS / Round Robin (Inserção O(1) na cauda) */
    if (!q->cmp) {
        if (!q->head) {
            q->head = new_node;
            q->tail = new_node;
        } else {
            q->tail->next = new_node;
            q->tail = new_node;
        }
        q->size++;
        return 1; /* 1 operação de inserção */
    }

    /* Caso 2: Fila Ordenada por Prioridade */
    if (!q->head) {
        q->head = new_node;
        q->tail = new_node;
        q->size++;
        return 1;
    }

    /* Inserção no início (maior prioridade que o head atual) */
    if (q->cmp(p, q->head->process) < 0) {
        new_node->next = q->head;
        q->head = new_node;
        q->size++;
        return 1; /* 1 comparação realizada */
    }

    /* Busca a posição correta mantendo a contagem de iterações */
    Node *current = q->head;
    size_t iterations = 1; /* Já comparou com a cabeça */

    while (current->next != NULL && q->cmp(p, current->next->process) >= 0) {
        current = current->next;
        iterations++;
    }

    new_node->next = current->next;
    current->next = new_node;

    /* Atualiza a cauda se foi inserido ao final */
    if (new_node->next == NULL) {
        q->tail = new_node;
    }

    q->size++;
    return iterations;
}

Process *queue_remove(ProcessQueue *q) {
    if (!q || q->size == 0 || !q->head) {
        return NULL;
    }

    /* Como a lista já é mantida ordenada na inserção,
     * a remoção é sempre O(1) removendo o head. */
    Node *removed_node = q->head;
    Process *p = removed_node->process;

    q->head = q->head->next;
    if (!q->head) {
        q->tail = NULL;
    }

    free(removed_node);
    q->size--;

    return p;
}

Process *queue_for_each(ProcessQueue *q, QueueIterator iter) {
    if (!q || !iter) return NULL;

    Node *current = q->head;
    while (current) {
        if (iter(current->process, NULL) != 0) {
            return current->process;
        }
        current = current->next;
    }

    return NULL;
}