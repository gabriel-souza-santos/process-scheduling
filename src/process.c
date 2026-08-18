#include <stdlib.h>
#include <stdint.h>
#include "process.h"

typedef enum {
    BURST_CPU,
    BURST_IO
} BurstType;

typedef struct {
    BurstType type;
    Duration duration;
} Burst;

struct Process {
    int id;
    int priority;
    Duration arrival_time;
    ProcessState state;
    Burst *bursts;
    size_t burst_count;
    size_t current_burst; /* índice da rajada atual */
    Duration burst_remaining; /* tempo restante na rajada atual */
};

Process *process_new(ProcessAttr attr) {
    Process *p = malloc(sizeof(Process));
    if (!p) return NULL;

    p->id = attr.id;
    p->priority = attr.priority;
    p->arrival_time = attr.arrival_time;
    p->state = PROCESS_READY;
    p->bursts = NULL;
    p->burst_count = 0;
    p->current_burst = 0;
    p->burst_remaining = 0;

    return p;
}

void process_destroy(Process **p) {
    if (p && *p) {
        free((*p)->bursts);
        free(*p);
        *p = NULL;
    }
}

int process_priority(const Process *p) {
    return p->priority;
}

void process_set_priority(Process *p, int new_priority) {
    p->priority = new_priority;
}

int process_id(const Process *p) {
    return p->id;
}

Duration process_arrival_time(const Process *p) {
    return p->arrival_time;
}

ProcessState process_state(const Process *p) {
    return p->state;
}

/* Adiciona uma rajada ao processo.
 * Alterna automaticamente entre CPU e E/S (CPU primeiro). */
int process_add_burst(Process *p, Duration duration) {
    Burst *new_bursts = realloc(p->bursts, (p->burst_count + 1) * sizeof(Burst));
    if (!new_bursts) return -1;

    p->bursts = new_bursts;
    BurstType type = (p->burst_count % 2 == 0) ? BURST_CPU : BURST_IO;
    p->bursts[p->burst_count] = (Burst){ .type = type, .duration = duration };

    if (p->burst_count == 0) {
        p->burst_remaining = duration; /* inicializa o contador da primeira rajada */
    }

    p->burst_count++;
    return 0;
}

/* Tenta executar o processo na CPU pelo tempo determinado (quantum).
 * Retorna o tempo realmente consumido; pode ser menor se a rajada de CPU
 * acabar antes.
 *
 * Se não houver mais rajadas de CPU, o processo é marcado como TERMINATED e
 * retorna 0. */
Duration process_execute(Process *p, Duration time) {
    if (p->state == PROCESS_TERMINATED) return 0;

    /* Garante que estamos em uma rajada de CPU */
    if (p->current_burst >= p->burst_count ||
        p->bursts[p->current_burst].type != BURST_CPU) {
        p->state = PROCESS_TERMINATED;
        return 0;
    }

    p->state = PROCESS_RUNNING;

    Duration consumed = (time < p->burst_remaining) ? time : p->burst_remaining;
    p->burst_remaining -= consumed;

    if (p->burst_remaining == 0) {
        p->current_burst++;

        if (p->current_burst >= p->burst_count) {
            /* Sem mais rajadas: processo finalizado */
            p->state = PROCESS_TERMINATED;
        } else {
            /* Próxima rajada é E/S */
            p->burst_remaining = p->bursts[p->current_burst].duration;
            p->state = PROCESS_WAITING;
        }
    } else {
        /* Rajada interrompida pelo quantum: volta para a fila (READY) */
        p->state = PROCESS_READY;
    }

    return consumed;
}

/* Tenta processar o tempo de E/S. Retorna o tempo realmente consumido;
 * pode ser menor se a rajada de E/S acabar antes. */
Duration process_wait(Process *p, Duration time) {
    if (p->state == PROCESS_TERMINATED) return 0;

    /* Garante que estamos em uma rajada de E/S */
    if (p->current_burst >= p->burst_count ||
        p->bursts[p->current_burst].type != BURST_IO) {
        return 0;
    }

    p->state = PROCESS_WAITING;

    Duration consumed = (time < p->burst_remaining) ? time : p->burst_remaining;
    p->burst_remaining -= consumed;

    if (p->burst_remaining == 0) {
        p->current_burst++;

        if (p->current_burst >= p->burst_count) {
            p->state = PROCESS_TERMINATED;
        } else {
            /* Próxima rajada é CPU: processo pronto novamente */
            p->burst_remaining = p->bursts[p->current_burst].duration;
            p->state = PROCESS_READY;
        }
    }

    return consumed;
}

/* Retorna o tempo total de CPU ainda necessário pelo processo. */
Duration process_remaining_cpu_time(Process *p) {
    Duration total = 0;
    for (size_t i = p->current_burst; i < p->burst_count; i++) {
        if (p->bursts[i].type == BURST_CPU) {
            total += (i == p->current_burst) ? p->burst_remaining
                                              : p->bursts[i].duration;
        }
    }
    return total;
}
