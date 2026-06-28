# Makefile — 05_ld_elfloader
#
# A user-space RISC-V ELF loader (mini dlopen/dlsym/dlclose).
# Both RV64 (default) and RV32 are supported, built into separate
# output directories so they can coexist.
#
#   make            build RV64  -> build/rv64/
#   make rv32       build RV32  -> build/rv32/
#   make run        build + run RV64 tests under qemu
#   make rv32-run   build + run RV32 tests under qemu
#   make check      show compiler / target info
#   make clean      remove build outputs
#
#   make DEBUG_LOG=1   rebuild with verbose [DEBUG] tracing

CROSS     ?= riscv64-unknown-linux-gnu-
CC        := $(CROSS)gcc
QEMU64    ?= qemu-riscv64
QEMU32    ?= qemu-riscv32
SYSROOT   := $(shell $(CC) -print-sysroot)
DEBUG_LOG ?= 0

# --- RV64 (default) ---
RV64_MARCH  ?= rv64imafd
RV64_MABI   ?= lp64d
RV64_CFLAGS  = -march=$(RV64_MARCH) -mabi=$(RV64_MABI) -DDEBUG_LOG=$(DEBUG_LOG)
RV64_QFLAGS  = -cpu rv64,g=true,c=true,v=true -L $(SYSROOT)

# --- RV32 ---
RV32_MARCH  ?= rv32imafd
RV32_MABI   ?= ilp32d
RV32_CFLAGS  = -march=$(RV32_MARCH) -mabi=$(RV32_MABI) -DDEBUG_LOG=$(DEBUG_LOG)
RV32_QFLAGS  = -cpu rv32,g=true,c=true,v=true -L $(SYSROOT)

.PHONY: all rv64 rv32 run rv64-run rv32-run check clean _build _run

all: rv64

rv64:
	@$(MAKE) --no-print-directory _build OUT=build/rv64 CFLAGS="$(RV64_CFLAGS)"

rv32:
	@$(MAKE) --no-print-directory _build OUT=build/rv32 CFLAGS="$(RV32_CFLAGS)"

run: rv64
	@$(MAKE) --no-print-directory _run OUT=build/rv64 \
		QEMU="$(QEMU64)" QFLAGS="$(RV64_QFLAGS)"

rv64-run: run

rv32-run: rv32
	@$(MAKE) --no-print-directory _run OUT=build/rv32 \
		QEMU="$(QEMU32)" QFLAGS="$(RV32_QFLAGS)"

# --- internal: build all programs into $(OUT) ---
_build: $(OUT)/elfloader $(OUT)/hello_dyn $(OUT)/hello_static

$(OUT)/elfloader: elf_loader.c main.c wheelc/list.c | $(OUT)
	$(CC) $(CFLAGS) -fPIE -o $@ $^ -ldl -lpthread -pie

# Freestanding static ET_EXEC (no libc): exercises the loader's
# section-header / static relocation path without needing TLS setup.
$(OUT)/hello_static: hello_static.c | $(OUT)
	$(CC) $(CFLAGS) -static -nostdlib -no-pie -e main -o $@ $<

$(OUT)/hello_dyn: hello.c | $(OUT)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(OUT):
	mkdir -p $@

# --- internal: run the two tests from $(OUT) ---
_run:
	@echo "=== Test 1: dynamic ELF ==="
	$(QEMU) $(QFLAGS) ./$(OUT)/elfloader ./$(OUT)/hello_dyn main; \
		echo "  exit code: $$?"
	@echo ""
	@echo "=== Test 2: static ELF ==="
	$(QEMU) $(QFLAGS) ./$(OUT)/elfloader ./$(OUT)/hello_static main; \
		echo "  exit code: $$?"

check:
	@echo "CC        = $(CC)"
	@echo "SYSROOT   = $(SYSROOT)"
	@echo "QEMU64    = $(QEMU64)"
	@echo "QEMU32    = $(QEMU32)"
	@echo "DEBUG_LOG = $(DEBUG_LOG)"
	@echo ""
	@echo "--- RV64 ($(RV64_CFLAGS)) ---"
	@$(CC) $(RV64_CFLAGS) -dM -E - < /dev/null | grep -E '__riscv_xlen|__LP64__'
	@echo "--- RV32 ($(RV32_CFLAGS)) ---"
	@$(CC) $(RV32_CFLAGS) -dM -E - < /dev/null | grep -E '__riscv_xlen|__LP64__'

clean:
	rm -rf build
