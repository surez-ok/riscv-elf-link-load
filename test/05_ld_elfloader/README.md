# 05_ld_elfloader — A user-space RISC-V ELF loader (mini dlopen)

A ~1500-line runtime ELF loader for RISC-V. It loads an ELF file (a shared
object or an executable) into the *current* process's address space, performs
symbol resolution and relocation itself, then runs a function from it by name
(or `main` for `ET_EXEC`). In essence it is a hand-written `dlopen` + `dlsym`
+ `dlclose`. Supports both RV64 and RV32.

## What it does

Unlike `03_simple_elfloader` — which only maps the segments and jumps,
delegating all linking to `ld.so` — this loader does the linking itself:

- **Loading** — verifies the ELF header, maps the loadable segments
  (`layout_segments`), and locates `PT_DYNAMIC`.
- **Dynamic-section parsing** — reads `DT_HASH`/`DT_GNU_HASH`, `DT_STRTAB`/
  `DT_SYMTAB`, `DT_RELA`/`DT_JMPREL`, `DT_NEEDED`, … (RELA only, as on RISC-V).
- **Symbol resolution** — SysV ELF hash and GNU hash lookup (linear fallback);
  searched locally first, then in `DT_NEEDED` dependencies, then falling back
  to `dlsym()` for system libraries (libc, libpthread, …).
- **Relocation** — applies RISC-V RELA types: `R_RISCV_32`/`R_RISCV_64`,
  `RELATIVE`, `COPY`, `JUMP_SLOT`, `IRELATIVE` (IFUNC), `PCREL_HI20/LO12_I/
  LO12_S`, `HI20/LO12_I/LO12_S`, `CALL/CALL_PLT`, `GOT_HI20`, `BRANCH`, `JAL`.
- **Dependencies** — for each `DT_NEEDED`, reuses an already-loaded module or
  `dlopen()`s the system library, remembering the handle for later `dlclose`.
- **init/fini arrays** — runs `.preinit_array`/`.init_array` before invoking
  the target function, and `.fini_array` on unload.
- **Static executables** — for a non-PIE static `ET_EXEC` (no dynamic section),
  reads `.symtab`/`.rela.*`/init-fini arrays from the section headers and
  applies `R_RISCV_RELATIVE`/`R_RISCV_IRELATIVE`.

> **Note:** the static path calls `main()` directly (it does not run `_start`
> / `__libc_start_main`), so it does not set up TLS. A glibc static binary
> therefore works only when its `printf` path happens not to touch
> TLS-dependent `malloc` (rv64) and crashes on rv32. The bundled
> `hello_static` is freestanding (no libc) to avoid this; see below.

## Directory layout

```
elf_loader.c     core loader (load / link / relocate / run)
elf_loader.h     module state and public API
main.c           CLI driver: ./elfloader <elf_file> <func>
hello.c          test: shared object with main() + worker() (uses pthread)
hello_static.c   test: freestanding static executable (raw write syscall)
wheelc/          small helper library (log.h, list.h/.c)
```

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
`CROSS=` if you have a dedicated RV32 toolchain.

```bash
# usage: load <elf_file> and call <func> in it
./build/rv64/elfloader <elf_file> <func>
```

## Test cases

| Binary | Built as | Loader path taken | Output |
|---|---|---|---|
| `hello_dyn` | `-fPIC -shared` (ET_DYN `.so`) | dynamic: resolve `pthread_create`/`sleep`/`printf` via `dlsym`, apply relocations, call `main` | `0: hello world!` … `9: hello world!` |
| `hello_static` | `-static -nostdlib -no-pie` (freestanding ET_EXEC) | static: read `.symtab` from section headers, resolve `main`, call it | `hello world!` |

```bash
make run
#  ./build/rv64/elfloader ./build/rv64/hello_dyn main
#    0: hello world! ... 9: hello world!
#    module hello_dyn run done
#
#  ./build/rv64/elfloader ./build/rv64/hello_static main
#    hello world!
#    module hello_static run done
```

For an `ET_EXEC` module the `<func>` argument is ignored (its `main` symbol is
resolved and called directly); for an `ET_DYN` module the named symbol is
looked up and invoked.

## Relationship to 03_simple_elfloader

| | 03_simple_elfloader | 05_ld_elfloader |
|---|---|---|
| Lines | ~590 | ~1500 |
| Linking | delegated to ld.so | done here |
| Symbol resolution | N | Y (ELF hash / GNU hash / linear) |
| Relocation | N | Y (RISC-V RELA types) |
| init/fini arrays | N | Y |
| DT_NEEDED deps | N (ld.so) | Y (dlopen) |
| RV32 support | N | Y |

## Debug logging

`LOG_DEBUG` tracing is compiled out by default. Rebuild with `DEBUG_LOG=1` to
re-enable it:

```bash
make clean && make DEBUG_LOG=1
```

`LOG_INFO` / `LOG_WARN` / `LOG_ERR` are always kept.

## License

Based on elemeta's elfloader (elemeta47 at gmail dot com), MIT License — see
`LICENSE`.
