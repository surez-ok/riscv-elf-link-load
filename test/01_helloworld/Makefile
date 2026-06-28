# 1. Configuration
STATIC_FLAGS := -static
SUFFIX := $(if $(STATIC_FLAGS),_static,)
TARGET := helloworld$(SUFFIX).elf

CC := riscv64-unknown-linux-gnu-gcc
QEMU_USER := qemu-riscv64
QEMU_SYSROOT := /study/riscv-gnu-toolchain/build_glibc/sysroot

SRC_DIRS := .
CFLAGS := -march=rv64imafdc -mabi=lp64d -O2 -g -Wall -Wextra
LDFLAGS := $(STATIC_FLAGS)

# 2. Automatically collect source files and generate object/dependency targets
SRCS := $(sort $(shell find $(SRC_DIRS) -name '*.c'))
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)  # List of dependency files

# 3. QEMU execution macro
define RUN_QEMU
$(QEMU_USER) -cpu rv64,g=true,c=true,v=true -L $(QEMU_SYSROOT) $1
endef

# 4. Build rules
all: build run

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Support subdirectory compilation and auto-generate dependency files (-MMD -MP)
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	@$(call RUN_QEMU,$(TARGET)); echo ""; echo "";

# 5. Clean up and phony targets
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET)

.PHONY: all build run clean

# 6. Include auto-generated dependency files (ignore non-existent files)
-include $(DEPS)
