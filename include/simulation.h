#ifndef SIMULATION_H
#define SIMULATION_H

#include "process.h"
#include "scheduler.h"

SchedulerMetrics simulation_run(Process **processes, size_t num_processes,
                    Scheduler scheduler);

#endif /* SIMULATION_H */