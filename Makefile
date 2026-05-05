# configure  — download and configure all dependencies (FetchContent), generate build system
# build      — run configure then compile the project and all dependencies
# test       — build then run all tests; prints output on any test failure
# run-tests  — re-run tests without rebuilding, use when source hasn't changed
# clean      — delete build directory; required when adding new dependencies or changing cache flags
# rebuild    — full clean + build from scratch
#
BUILD_DIR := build
JOBS      := $(shell nproc)

.PHONY: configure build test run-tests clean rebuild

configure:
	cmake -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run-tests:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build
