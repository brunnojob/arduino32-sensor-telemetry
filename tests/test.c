#include "monitor.h"
#include <assert.h>
#include <math.h>
static int fail(void* context, const wire_sample* sample) { (void)context; (void)sample; return 0; }
int main(void) {
    uint8_t frame[WIRE_FRAME_SIZE]; wire_sample decoded;
    wire_sample sample = {0, 2000, -1250, 5000, 1, 1};
    wire_encode(&sample, frame);
    assert(wire_decode(frame, &decoded) && decoded.temperature == -1250);
    for (size_t bit = 0; bit < sizeof frame * 8; ++bit) {
        frame[bit / 8] ^= (uint8_t)(1u << (bit % 8));
        assert(!wire_decode(frame, &decoded));
        frame[bit / 8] ^= (uint8_t)(1u << (bit % 8));
    }
    wire_parser parser = {{0}, 0};
    for (unsigned i = 0; i < 77; ++i) assert(!wire_feed(&parser, 0x55, &decoded));
    int received = 0;
    for (size_t i = 0; i < sizeof frame; ++i) received += wire_feed(&parser, frame[i], &decoded);
    assert(received == 1 && decoded.sequence == 0);
    sensor_monitor monitor = {0}; monitor.high = 3500; monitor.hysteresis = 200;
    sample.temperature = 3600;
    assert(monitor_accept(&monitor, &sample) == 1 && monitor.alarm);
    assert(monitor_accept(&monitor, &sample) == 0 && monitor.duplicates == 1);
    sample.sequence = 2; sample.uptime = 6000; sample.temperature = 3400;
    assert(monitor_accept(&monitor, &sample) == 1 && monitor.lost == 1 && monitor.alarm);
    sample.sequence = 3; sample.uptime = 8000; sample.temperature = 3200;
    assert(monitor_accept(&monitor, &sample) == 1 && !monitor.alarm);
    assert(fabs(monitor.mean_temperature - 34) < 1e-9);
    assert(fabs(monitor_variance(&monitor) - 4) < 1e-9);
    sample.sequence = 0; sample.uptime = 2000;
    assert(monitor_accept(&monitor, &sample) == 1 && monitor.restarts == 1);
    monitor.repository.write = fail; sample.sequence = 1; sample.uptime = 4000;
    assert(monitor_accept(&monitor, &sample) == -1 && monitor.last_sequence == 0);
}
