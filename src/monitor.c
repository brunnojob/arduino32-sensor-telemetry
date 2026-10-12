#include "monitor.h"
int monitor_accept(sensor_monitor* monitor, const wire_sample* sample) {
    if (monitor->initialized && sample->sequence == monitor->last_sequence && sample->uptime == monitor->last_uptime) {
        ++monitor->duplicates; return 0;
    }
    sensor_monitor next = *monitor;
    if (next.initialized) {
        const uint32_t delta = sample->sequence - next.last_sequence;
        const uint32_t elapsed = sample->uptime - next.last_uptime;
        if (delta >= 0x80000000u || elapsed >= 0x80000000u) {
            if (sample->sequence > 2 || sample->uptime > 10000) { ++monitor->invalid; return 0; }
            ++next.restarts;
        } else {
            if (delta == 0) { ++monitor->invalid; return 0; }
            next.lost += delta - 1;
        }
    }
    next.initialized = 1; next.last_sequence = sample->sequence; next.last_uptime = sample->uptime;
    ++next.received;
    if (!sample->valid) ++next.invalid;
    else {
        ++next.valid_count;
        const double value = sample->temperature / 100.0;
        const double delta = value - next.mean_temperature;
        next.mean_temperature += delta / (double)next.valid_count;
        next.m2_temperature += delta * (value - next.mean_temperature);
        if (sample->temperature >= next.high) next.alarm = 1;
        else if ((int32_t)sample->temperature <= (int32_t)next.high - next.hysteresis) next.alarm = 0;
    }
    if (next.repository.write && !next.repository.write(next.repository.context, sample)) return -1;
    *monitor = next;
    return 1;
}
double monitor_variance(const sensor_monitor* monitor) {
    return monitor->valid_count > 1 ? monitor->m2_temperature / (double)(monitor->valid_count - 1) : 0;
}
