#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Process Process;

/* Representação de duração de tempo arbitrária.
 Caso necessário, considere com nanosegundos */
typedef size_t Duration;

typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} ProcessState;

/* Atrributos para criação de processos */
typedef struct {
    int priority; /* quanto maior o valor, mais prioridade o processo tem*/
    int id; /* deve ser único para cada processo */
    Duration arrival_time; /* tempo de chegada do processo */
} ProcessAttr;

Process *process_new(ProcessAttr attr);
void process_destroy(Process **p);

int process_priority(Process *p);
int process_id(Process *p);

Duration process_arrival_time(Process *p);
ProcessState process_state(Process *p);

/* Tenta executar o processo na CPU pelo tempo de determinado (quantum).
 * Retorna o tempo realmente consumido, pode ser menor se a rajada de CPU
 * acabar antes. */
Duration process_execute(Process *p, Duration time);

/* Tenta processar o tempo de E/S. Retorna o tempo realmente consumido,
 * pode ser menor se a rajada de E/S acabar antes. */
Duration process_wait(Process *p, Duration time);

/* Adiciona uma rajada (CPU ou E/S alternados) ao processo.
 * A primeira rajada é sempre CPU. Retorna 0 em caso de sucesso. */
int process_add_burst(Process *p, Duration duration);

/* Retorna o tempo total de CPU ainda necessário pelo processo. */
Duration process_remaining_cpu_time(Process *p);

#endif /* PROCESS_H */