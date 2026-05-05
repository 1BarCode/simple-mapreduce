BUILD_DIR := build
JOBS      := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu)

.PHONY: configure build test clean rebuild

configure:
	cmake -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build
