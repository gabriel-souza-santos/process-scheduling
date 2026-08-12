#include "process.h"

typedef enum {
    BURST_CPU,
    BURST_IO
} BurstType;

typedef struct {
    BurstType type;
    Duration duration;
} Burst;

struct Process {
    int id;
    int priority;
    Duration arrival_time;
    ProcessState state;
    Burst *bursts;
    size_t burst_count;
};


