#include "process_queue.h"

struct ProcessQueue {
    size_t capacity;
    size_t size;
    size_t front;
    size_t rear;
    Process **items;
    ProcessComparator cmp_func;
};

