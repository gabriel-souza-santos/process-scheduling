#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "process_queue.h"
#include <stddef.h>

typedef struct {
    double average_turnaround;
    double average_slowdown;
    double jain_index;
    double context_switches;
    double num_processes;
    double total_comparisons;
    double total_time;
} SchedulerMetrics;

typedef struct Scheduler Scheduler;

typedef size_t (*EnqueuePolicy)(Scheduler *s, ProcessControlBlock *pcb, Duration current_time);
typedef Process *(*DispatchPolicy)(Scheduler *s, ProcessControlBlock pcb_table[], size_t size, Duration current_time);

struct Scheduler {
    ProcessQueue *queue;
    Duration quantum;
    EnqueuePolicy enqueue;
    DispatchPolicy dispatch;
};

void metrics_print(SchedulerMetrics metrics, const char *algorithm_name);

#endif /* SCHEDULER_H */