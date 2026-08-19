#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Process Process;

/* Representação de duração de tempo arbitrária.
 Caso necessário, considere com nanosegundos */
typedef size_t Duration;

/* Representação de duração de tempo infinito */
#define DURATION_INF ((Duration)(-1))

typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} ProcessState;

/* Atrributos para criação de processos */
typedef struct {
    int id;                 /* incremental e único para cada processo */
    int priority;           /* quanto maior o valor, mais prioridade o processo tem*/
    Duration arrival_time;  /* tempo de chegada do processo */
} ProcessAttr;

typedef struct {
    Process *process;       /* O processo em si */
    Duration wake_time;     /* Quando deve acordar de E/S (DURATION_INF se não estiver em E/S) */
    Duration finish_time;   /* Instante em que concluiu toda a execução */
    Duration cpu_time;      /* Tempo total de CPU consumido */
    Duration burst_time;    /* Tempo acumulado APENAS no burst de CPU atual */
    size_t burst_count;     /* Número de bursts de CPU já iniciados/executados */
    int base_priority;      /* Prioridade original */
} ProcessControlBlock;

Process *process_new(ProcessAttr attr);
void process_destroy(Process **p);

int process_priority(const Process *p);
void process_set_priority(Process *p, int new_priority);
int process_id(const Process *p);

Duration process_arrival_time(const Process *p);
ProcessState process_state(const Process *p);

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