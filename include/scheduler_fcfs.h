#ifndef SCHEDULER_FCFS_H
#define SCHEDULER_FCFS_H

#include "scheduler.h"

Scheduler *fcfs_new(void);
void fcfs_destroy(Scheduler *s);

#endif /* SCHEDULER_FCFS_H */