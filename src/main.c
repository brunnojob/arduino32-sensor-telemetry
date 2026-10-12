#define _POSIX_C_SOURCE 200809L
#include "monitor.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
static volatile sig_atomic_t running = 1;
static void stop(int value) { (void)value; running = 0; }
static int csv_write(void* context, const wire_sample* sample) {
    FILE* stream = context;
    return fprintf(stream, "%u,%u,%d,%u,%u,%u\n", sample->sequence, sample->uptime, sample->temperature, sample->humidity, sample->presence, sample->valid) > 0 && fflush(stream) == 0;
}
int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "usage: telemetry SERIAL_DEVICE\n"); return 2; }
    int fd = open(argv[1], O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) { perror("open"); return 1; }
    struct termios saved, config;
    if (tcgetattr(fd, &saved) < 0) { perror("tcgetattr"); close(fd); return 1; }
    config = saved;
    config.c_iflag = 0; config.c_oflag = 0; config.c_lflag = 0;
    config.c_cflag = CS8 | CREAD | CLOCAL;
    config.c_cc[VMIN] = 0; config.c_cc[VTIME] = 0;
    if (cfsetispeed(&config, B115200) || cfsetospeed(&config, B115200) || tcsetattr(fd, TCSANOW, &config)) { perror("serial"); close(fd); return 1; }
    signal(SIGINT, stop); signal(SIGTERM, stop);
    wire_parser parser = {{0}, 0};
    sensor_monitor monitor = {0}; monitor.high = 3500; monitor.hysteresis = 200;
    monitor.repository.write = csv_write; monitor.repository.context = stdout;
    puts("sequence,uptime_ms,temperature_centi_c,humidity_centi_percent,presence,valid");
    int result = 0;
    while (running) {
        struct pollfd item = {fd, POLLIN, 0};
        int ready = poll(&item, 1, 1000);
        if (ready < 0) { if (errno == EINTR) continue; perror("poll"); result = 1; break; }
        if (item.revents & (POLLERR | POLLHUP | POLLNVAL)) { fprintf(stderr, "serial disconnected\n"); result = 1; break; }
        if (!(item.revents & POLLIN)) continue;
        uint8_t bytes[256]; ssize_t count = read(fd, bytes, sizeof bytes);
        if (count < 0) { if (errno == EAGAIN || errno == EINTR) continue; perror("read"); result = 1; break; }
        for (ssize_t i = 0; i < count; ++i) {
            wire_sample sample;
            if (wire_feed(&parser, bytes[i], &sample) && monitor_accept(&monitor, &sample) < 0) { fprintf(stderr, "repository write failed\n"); result = 1; running = 0; break; }
        }
    }
    fprintf(stderr, "received=%llu lost=%llu duplicates=%llu invalid=%llu restarts=%llu alarm=%d mean=%.3f variance=%.3f\n", (unsigned long long)monitor.received, (unsigned long long)monitor.lost, (unsigned long long)monitor.duplicates, (unsigned long long)monitor.invalid, (unsigned long long)monitor.restarts, monitor.alarm, monitor.mean_temperature, monitor_variance(&monitor));
    tcsetattr(fd, TCSANOW, &saved); close(fd); return result;
}
