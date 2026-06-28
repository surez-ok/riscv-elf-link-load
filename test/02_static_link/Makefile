# 1. Configuration
TARGET := static_link.elf

CC := riscv64-unknown-linux-gnu-gcc

SRC_DIRS := .
LDSCRIPT := linker_1.lds
CFLAGS := -march=rv64imafdc -mabi=lp64d -O0
LDFLAGS := -static -nostdlib -Wl,--no-warn-rwx-segments -T $(LDSCRIPT)

# 2. Automatically collect source files and generate object/dependency targets
SRCS := $(sort $(shell find $(SRC_DIRS) -name '*.c'))
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)  # List of dependency files

# 4. Build rules
all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Support subdirectory compilation and auto-generate dependency files (-MMD -MP)
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# 5. Clean up and phony targets
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET)

.PHONY: all build clean

# 6. Include auto-generated dependency files (ignore non-existent files)
-include $(DEPS)
