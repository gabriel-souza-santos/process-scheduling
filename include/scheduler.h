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

typedef size_t (*EnqueuePolicy)(ProcessQueue *q, Process *p);
typedef Process *(*DispatchPolicy)(ProcessQueue *q);

typedef struct {
    ProcessQueue *queue; 
    Duration quantum;
    EnqueuePolicy enqueue;
    DispatchPolicy dispatch;
} Scheduler;

void metrics_print(SchedulerMetrics metrics, const char *algorithm_name);
void metrics_export(const char *filepath, SchedulerMetrics metrics,
                    const char *algorithm_name, const char *scenario);

#endif /* SCHEDULER_H */