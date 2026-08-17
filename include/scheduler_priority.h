#ifndef SCHEDULER_PRIORITY_H
#define SCHEDULER_PRIORITY_H

#include "scheduler.h"

Scheduler priority_new(void);
void priority_destroy(Scheduler *s);

#endif /* SCHEDULER_PRIORITY_H */