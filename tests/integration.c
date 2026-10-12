#define _XOPEN_SOURCE 600
#include "wire.h"
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
int main(void) {
    int master = posix_openpt(O_RDWR | O_NOCTTY); assert(master >= 0);
    assert(grantpt(master) == 0 && unlockpt(master) == 0);
    char* device = ptsname(master); assert(device);
    int output[2], errors[2]; assert(pipe(output) == 0 && pipe(errors) == 0);
    pid_t pid = fork(); assert(pid >= 0);
    if (pid == 0) {
        close(master); close(output[0]); close(errors[0]);
        dup2(output[1], STDOUT_FILENO); dup2(errors[1], STDERR_FILENO);
        close(output[1]); close(errors[1]);
        execl("./build/telemetry", "telemetry", device, (char*)0); _exit(127);
    }
    close(output[1]); close(errors[1]);
    usleep(100000);
    wire_sample sample = {0, 2000, 2534, 6500, 1, 1};
    uint8_t frame[WIRE_FRAME_SIZE]; wire_encode(&sample, frame);
    assert(write(master, frame, sizeof frame) == sizeof frame);
    struct pollfd item = {output[0], POLLIN, 0};
    assert(poll(&item, 1, 2000) > 0);
    char bytes[1024] = {0}; ssize_t count = read(output[0], bytes, sizeof bytes - 1);
    assert(count > 0);
    unsigned attempts = 0;
    while (!strstr(bytes, "0,2000,2534,6500,1,1") && attempts++ < 5) {
        assert(poll(&item, 1, 2000) > 0);
        ssize_t next = read(output[0], bytes + count, sizeof bytes - 1 - (size_t)count);
        assert(next > 0); count += next;
    }
    const int accepted = strstr(bytes, "0,2000,2534,6500,1,1") != 0;
    kill(pid, SIGINT); int status = 0; assert(waitpid(pid, &status, 0) == pid);
    close(master); close(output[0]); close(errors[0]);
    assert(accepted && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
