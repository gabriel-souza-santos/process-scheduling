#ifndef SCHEDULER_ROUND_ROBIN_H
#define SCHEDULER_ROUND_ROBIN_H

#include "scheduler.h"

Scheduler round_robin_new(Duration quantum);
void round_robin_destroy(Scheduler *s);

#endif /* SCHEDULER_ROUND_ROBIN_H */