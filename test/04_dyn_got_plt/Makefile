# 1. Configuration
TARGET := dyn_got_plt.elf
LIBS   := add.so sub.so

CC := riscv64-unknown-linux-gnu-gcc
OBJDUMP := riscv64-unknown-linux-gnu-objdump
QEMU_USER := qemu-riscv64
QEMU_SYSROOT := $(shell $(CC) -print-sysroot)

CFLAGS := -march=rv64imafdc -mabi=lp64d -O2 -g -Wall -Wextra

# Filter LD_DEBUG=bindings output down to the symbols this example cares about
BIND_GREP := grep -E "symbol \`(add|sub|g_sum|g_sub)'|calling init"

# 2. QEMU execution macro (let the dynamic loader find add.so / sub.so)
define RUN_QEMU
$(QEMU_USER) -cpu rv64,g=true,c=true,v=true -L $(QEMU_SYSROOT) -E LD_LIBRARY_PATH=. $1
endef

# 3. Build rules
all: build run

build: $(TARGET) $(LIBS)

# main.c -> PIE executable, dynamically linked against add.so / sub.so
$(TARGET): main.c $(LIBS)
	$(CC) $(CFLAGS) -fPIE -pie $< -o $@ $(LIBS) -Wl,-rpath,'$$ORIGIN'

# add.c / sub.c -> position-independent shared libraries
%.so: %.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

run: $(TARGET)
	@$(call RUN_QEMU,$(TARGET)); echo ""; echo "";

# Disassemble the final ELF (with inline relocations, useful for GOT/PLT)
DASM_FILE := $(TARGET:.elf=.dasm)

dasm: $(TARGET)
	$(OBJDUMP) -dr -M no-aliases $< > $(DASM_FILE)

# Observe lazy vs eager binding: LD_DEBUG=bindings shows when each symbol resolves
bind: $(TARGET)
	@echo "=== 默认（延迟绑定）：add/sub 在 calling init 之后、首次调用时才解析 ==="
	@$(QEMU_USER) -cpu rv64,g=true,c=true,v=true -L $(QEMU_SYSROOT) -E LD_LIBRARY_PATH=. -E LD_DEBUG=bindings $(TARGET) 2>&1 | $(BIND_GREP)
	@echo ""
	@echo "=== LD_BIND_NOW=1（立即绑定）：add/sub 在 calling init 之前、启动时全部解析 ==="
	@$(QEMU_USER) -cpu rv64,g=true,c=true,v=true -L $(QEMU_SYSROOT) -E LD_LIBRARY_PATH=. -E LD_BIND_NOW=1 -E LD_DEBUG=bindings $(TARGET) 2>&1 | $(BIND_GREP)

# 4. Clean up and phony targets
clean:
	-rm -f $(TARGET) $(LIBS) $(DASM_FILE)

.PHONY: all build run dasm bind clean
