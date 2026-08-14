#include "arrival_generator.h"

#include <stdlib.h>

#include "seed_generator.h"

bool arrival_config_validate(const ArrivalConfig *config) {
  if (!config)
    return false;

  switch (config->model) {
  case ARRIVAL_ALL_AT_ZERO:
    return true;

  case ARRIVAL_FIXED_INTERVAL:
    /* fixed_interval == 0 é aceito: degenera para "todos em 0". */
    return true;

  case ARRIVAL_RANDOM_INTERVAL:
    return config->min_interval <= config->max_interval;

  case ARRIVAL_BATCH:
    return config->batch_size >= 1;

  default:
    return false;
  }
}

static Duration *alloc_arrivals(size_t process_count) {
  return malloc(sizeof(Duration) * process_count);
}

static Duration *generate_all_at_zero(size_t process_count) {
  Duration *arrivals = alloc_arrivals(process_count);
  if (!arrivals)
    return NULL;

  for (size_t i = 0; i < process_count; i++) {
    arrivals[i] = 0;
  }
  return arrivals;
}

static Duration *generate_fixed_interval(const ArrivalConfig *config,
                                         size_t process_count) {
  Duration *arrivals = alloc_arrivals(process_count);
  if (!arrivals)
    return NULL;

  for (size_t i = 0; i < process_count; i++) {
    arrivals[i] = (Duration)i * config->fixed_interval;
  }
  return arrivals;
}

static Duration *generate_random_interval(const ArrivalConfig *config,
                                          size_t process_count) {
  Duration *arrivals = alloc_arrivals(process_count);
  if (!arrivals)
    return NULL;

  /* O primeiro processo sempre chega em t=0: ele "abre" a simulação.
   * Os demais chegam após um intervalo aleatório em relação ao
   * anterior, amostrado uniformemente em [min_interval, max_interval]. */
  arrivals[0] = 0;
  for (size_t i = 1; i < process_count; i++) {
    int gap = rng_between((int)config->min_interval, (int)config->max_interval);
    arrivals[i] = arrivals[i - 1] + (Duration)gap;
  }
  return arrivals;
}

static Duration *generate_batch(const ArrivalConfig *config,
                                size_t process_count) {
  Duration *arrivals = alloc_arrivals(process_count);
  if (!arrivals)
    return NULL;

  for (size_t i = 0; i < process_count; i++) {
    size_t batch_index = i / config->batch_size;
    arrivals[i] = (Duration)batch_index * config->batch_interval;
  }
  return arrivals;
}

Duration *arrival_generate(const ArrivalConfig *config, size_t process_count) {
  if (process_count == 0 || !arrival_config_validate(config))
    return NULL;

  /* Reinicializa o RNG global para garantir que a mesma seed sempre
   * produza a mesma sequência de chegadas, independentemente do que
   * tenha sido consumido do RNG antes desta chamada. */
  rng_seed(config->seed);

  switch (config->model) {
  case ARRIVAL_ALL_AT_ZERO:
    return generate_all_at_zero(process_count);
  case ARRIVAL_FIXED_INTERVAL:
    return generate_fixed_interval(config, process_count);
  case ARRIVAL_RANDOM_INTERVAL:
    return generate_random_interval(config, process_count);
  case ARRIVAL_BATCH:
    return generate_batch(config, process_count);
  default:
    return NULL;
  }
}
