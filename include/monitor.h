#ifndef SENSOR_MONITOR_H
#define SENSOR_MONITOR_H
#include "wire.h"
typedef int (*sample_writer)(void*, const wire_sample*);
typedef struct { sample_writer write; void* context; } sample_repository;
typedef struct {
    uint32_t last_sequence;
    uint32_t last_uptime;
    uint64_t received;
    uint64_t lost;
    uint64_t duplicates;
    uint64_t invalid;
    uint64_t restarts;
    double mean_temperature;
    double m2_temperature;
    uint64_t valid_count;
    int alarm;
    int initialized;
    int16_t high;
    int16_t hysteresis;
    sample_repository repository;
} sensor_monitor;
int monitor_accept(sensor_monitor*, const wire_sample*);
double monitor_variance(const sensor_monitor*);
#endif
