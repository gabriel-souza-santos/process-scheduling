#ifndef SCHEDULER_CUSTOM_H
#define SCHEDULER_CUSTOM_H

#include "scheduler.h"
#include <stddef.h>

Scheduler *custom_scheduler_new(Process *processes[], size_t size);
void custom_scheduler_destroy(Scheduler *s);

#endif /* SCHEDULER_CUSTOM_H */