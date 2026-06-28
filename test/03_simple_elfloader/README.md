# 03_simple_elfloader — Minimal user-space ELF loader

A ~590-line RISC-V user-space ELF loader that supports both RV32 and RV64
with the same code.

- **Dynamically-linked ELF** (ET_DYN + PT_INTERP): loads ld.so, builds the
  stack and auxv, and delegates linking to ld.so
- **Statically-linked ELF** (static-pie, ET_DYN): jumps directly to the
  program entry point, no ld.so needed

## Design

Mimics the Linux kernel's `binfmt_elf.c`:

1. Parse the ELF header and program header table
2. mmap the PT_LOAD segments (pages shared by segments are mapped once and
   take the union of their protections; BSS pages are zero-filled)
3. If PT_INTERP is present, load the interpreter (ld.so)
4. Build a Linux-compliant initial stack (argc / argv / envp / auxv)
5. Jump: dynamic → ld.so entry, static → program's own entry

The `ElfW()` macro automatically adapts pointer size, auxv structure, and
register width between 32/64-bit at compile time.

## Build & run

Both architectures build into separate output directories (`build/rv64/`,
`build/rv32/`) so they can coexist. RV64 is the default.

```bash
make            # build RV64  -> build/rv64/
make rv32       # build RV32  -> build/rv32/
make run        # build + run RV64 tests under qemu
make rv32-run   # build + run RV32 tests under qemu
make check      # show toolchain / target info
make clean      # remove build outputs
```

RV32 uses the riscv64 toolchain's multilib support by default; override
`CROSS` if you have a dedicated RV32 toolchain:

```bash
make rv32 CROSS=riscv32-unknown-linux-gnu-
```

The loader prints `[loader] ...` informational logs by default; build with
`DEBUG=0` to silence them (error messages are unaffected):

```bash
make run DEBUG=0
```

Example output (`make run`, RV64 — addresses vary due to ASLR):

```
[loader] interpreter: /lib/ld-linux-riscv64-lp64d.so.1
[loader] target  entry:  0x7f0482b1a5f0
[loader] target  phdr:   0x7f0482b1a040
[loader] interp  base:  0x7f04813c2000  entry: 0x7f04813d1ade
[loader] sp:             0x7f04813c1d00
[loader] jumping to ld.so...
Hello from dynamically-linked ELF!
  argv[0] = ./build/rv64/hello_dyn
  argv[1] = hello
  argv[2] = world
  exit code: 42
```

Example output (`make rv32-run`, RV32):

```
[loader] interpreter: /lib/ld-linux-riscv32-ilp32d.so.1
[loader] target  entry:  0x2b2d2420
[loader] target  phdr:   0x2b2d2034
[loader] interp  base:  0x2b42e000  entry: 0x2b43d312
[loader] sp:             0x2bc50e70
[loader] jumping to ld.so...
Hello from dynamically-linked ELF!
  argv[0] = ./build/rv32/hello_dyn
  argv[1] = hello
  argv[2] = world
  exit code: 42
```

## Architecture adaptation

Only a few macros matter; the rest of the code is bit-width agnostic:

```c
#if __LP64__
#  define ElfW(what)    Elf64_##what       // Elf64_Ehdr, Elf64_Phdr, ...
#  define ELF_CLASS     ELFCLASS64
#else
#  define ElfW(what)    Elf32_##what
#  define ELF_CLASS     ELFCLASS32
#endif

typedef ElfW(Addr) addr_t;   // uint32_t or uint64_t
```

Stack pointers (`addr_t`), auxv entries (`ElfW(auxv_t)`), and mmap parameter
sizes all follow automatically.

