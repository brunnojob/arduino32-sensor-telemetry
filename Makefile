CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Wpedantic -O2 -Iinclude
all: build/telemetry
build:
	mkdir -p build
build/telemetry: src/main.c src/monitor.c include/monitor.h include/wire.h | build
	$(CC) $(CFLAGS) src/main.c src/monitor.c -o $@
build/test: tests/test.c src/monitor.c include/monitor.h include/wire.h | build
	$(CC) $(CFLAGS) tests/test.c src/monitor.c -lm -o $@
test: build/test
	./build/test
clean:
	rm -rf build
.PHONY: all test clean

FQBN ?= esp32:esp32:esp32
firmware:
	arduino-cli compile --fqbn $(FQBN) firmware/sensor_node
.PHONY: firmware

build/integration: tests/integration.c include/wire.h | build
	$(CC) $(CFLAGS) tests/integration.c -o $@
integration: all build/integration
	./build/integration

.PHONY: integration
