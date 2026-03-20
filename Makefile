TARGET = bridge
BUILD_DIR = build

all: build

build:
	@cmake -S . -B $(BUILD_DIR)
	@cmake --build $(BUILD_DIR) -j$(nproc)

run: build
	@./$(BUILD_DIR)/$(TARGET)

clean:
	@rm -rf $(BUILD_DIR)

rebuild: clean build

rerun:
	@./$(BUILD_DIR)/$(TARGET)

.PHONY: all build run clean rebuild rerun debug release install