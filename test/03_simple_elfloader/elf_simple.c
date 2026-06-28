/*
 * elf_simple.c — RISC-V 32/64 user-space ELF loader (~590 lines)
 *
 * Adapts to RV32 / RV64 at compile time via the ElfW() macro.
 * Supports ET_EXEC and ET_DYN, delegating dynamic linking to ld.so.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <sys/auxv.h>
#include <elf.h>
#include <errno.h>

/* Debug/info logging: compile with -DDEBUG=0 to silence the [loader]
 * informational messages (error messages on stderr are always kept). */
#ifndef DEBUG
#  define DEBUG 1
#endif

#define PRINT_DEBUG(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "[loader] " fmt "\n", ##__VA_ARGS__); } while (0)

/* ================================================================
 *  Architecture adaptation macros
 * ================================================================ */
#if __LP64__
#  define ElfW(what)    Elf64_##what
#  define ELFW(what)    ELF64_##what
#  define ELF_CLASS     ELFCLASS64
#  define ELF_MACHINE   EM_RISCV
#else
#  define ElfW(what)    Elf32_##what
#  define ELFW(what)    ELF32_##what
#  define ELF_CLASS     ELFCLASS32
#  define ELF_MACHINE   EM_RISCV
#endif

#define PAGE_SIZE       4096UL
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#define ALIGN_DOWN(x)   ((x) & PAGE_MASK)
#define ALIGN_UP(x)     ALIGN_DOWN((x) + PAGE_SIZE - 1)

/* ================================================================
 *  Types
 * ================================================================ */
typedef ElfW(Addr)   addr_t;       /* architecture-dependent address type */

/* result of a single load */
typedef struct {
    addr_t base;
    addr_t entry;
    addr_t load_bias;
    addr_t phdr_addr;
} LoadResult;

/* loader global state */
typedef struct {
    LoadResult target;
    int        has_interp;
    LoadResult interp;
    uint16_t   phnum;
} State;

typedef struct {
    void   *addr;
    size_t len;
} MappedRange;

static void unmap_ranges(MappedRange *ranges, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (ranges[i].addr != NULL)
            munmap(ranges[i].addr, ranges[i].len);
    }
}

static int record_range(MappedRange **ranges, size_t *count, size_t *cap,
                        void *addr, size_t len)
{
    if (*count == *cap) {
        size_t nc = *cap ? *cap * 2 : 16;
        MappedRange *p = realloc(*ranges, nc * sizeof(*p));
        if (!p)
            return -1;
        *ranges = p;
        *cap = nc;
    }
    (*ranges)[(*count)++] = (MappedRange){ addr, len };
    return 0;
}

/* ================================================================
 *  ELF header / program header table
 * ================================================================ */
static int read_elf_headers(int fd, ElfW(Ehdr) *ehdr,
                            ElfW(Phdr) **ph_out, int *phnum_out)
{
    if (read(fd, ehdr, sizeof(*ehdr)) != (ssize_t)sizeof(*ehdr))
        return -1;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0
        || ehdr->e_ident[EI_CLASS] != ELF_CLASS
        || ehdr->e_machine != ELF_MACHINE
        || (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
        || ehdr->e_ehsize != sizeof(ElfW(Ehdr)))
        return -1;

    struct stat st;
    if (fstat(fd, &st) != 0)
        return -1;

    uint16_t phnum = ehdr->e_phnum;
    if (phnum == PN_XNUM) {
        /* extended program-header count lives in section header 0 */
        ElfW(Shdr) sh0;
        if (ehdr->e_shoff == 0
            || pread(fd, &sh0, sizeof(sh0), ehdr->e_shoff) != (ssize_t)sizeof(sh0))
            return -1;
        phnum = (uint16_t)sh0.sh_info;
    }
    if (phnum < 1 || phnum > 256)
        return -1;
    if (ehdr->e_phentsize != sizeof(ElfW(Phdr)))
        return -1;

    uint64_t phoff = (uint64_t)ehdr->e_phoff;
    uint64_t phsize = (uint64_t)phnum * sizeof(ElfW(Phdr));
    if (phoff + phsize > (uint64_t)st.st_size)
        return -1;

    ElfW(Phdr) *ph = malloc((size_t)phsize);
    if (!ph)
        return -1;
    if (pread(fd, ph, phsize, phoff) != (ssize_t)phsize) {
        free(ph);
        return -1;
    }

    *ph_out = ph;
    *phnum_out = phnum;
    return 0;
}

/* ================================================================
 *  ELF segment loading
 *
 *  Pages shared by multiple PT_LOAD segments are mapped once and take
 *  the union of their protections, mirroring binfmt_elf.c.  File-backed
 *  pages are mmap'd from the file; BSS pages are anonymous zero pages.
 * ================================================================ */
static int load_elf(int fd, const ElfW(Ehdr) *ehdr, const ElfW(Phdr) *ph,
                    int phnum, LoadResult *res)
{
    /* Pass 1: compute the address range */
    addr_t min_vaddr = (addr_t)-1, max_vaddr = 0;
    for (int i = 0; i < phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;
        if (ph[i].p_memsz > (addr_t)-1 - ph[i].p_vaddr)   /* wrap guard */
            return -1;
        if (ph[i].p_vaddr < min_vaddr)
            min_vaddr = ph[i].p_vaddr;
        addr_t end = ph[i].p_vaddr + ph[i].p_memsz;
        if (end > max_vaddr)
            max_vaddr = end;
    }
    if (min_vaddr == (addr_t)-1) {
        fprintf(stderr, "[loader] no PT_LOAD segments\n");
        return -1;
    }
    if (max_vaddr > (addr_t)-1 - (PAGE_SIZE - 1)) {   /* ALIGN_UP would wrap */
        fprintf(stderr, "[loader] segment range overflows address space\n");
        return -1;
    }
    min_vaddr = ALIGN_DOWN(min_vaddr);
    max_vaddr = ALIGN_UP(max_vaddr);
    size_t total_size = (size_t)(max_vaddr - min_vaddr);
    size_t npages = total_size / PAGE_SIZE;

    /* Determine the base address.  For ET_DYN keep the whole range reserved
     * (PROT_NONE) and MAP_FIXED the segments on top of it, so there is no
     * reserve-then-unmap race. */
    addr_t base, load_bias;
    void *reserve = NULL;
    if (ehdr->e_type == ET_EXEC) {
        base      = 0;
        load_bias = 0;
    } else {
        reserve = mmap(NULL, total_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (reserve == MAP_FAILED) {
            perror("mmap reserve");
            return -1;
        }
        base      = (addr_t)(uintptr_t)reserve;
        load_bias = base - min_vaddr;
    }
    addr_t range_start = min_vaddr + load_bias;   /* == base for ET_DYN */

    /* per-page bookkeeping: protection union, file backing, file offset */
    uint8_t  *pg_prot = calloc(npages, sizeof(uint8_t));
    uint8_t  *pg_file = calloc(npages, sizeof(uint8_t));
    uint64_t *pg_off  = calloc(npages, sizeof(uint64_t));
    MappedRange *mapped = NULL;
    size_t mapped_count = 0, mapped_cap = 0;
    if (!pg_prot || !pg_file || !pg_off) {
        fprintf(stderr, "[loader] out of memory\n");
        goto fail_reserve;
    }

    for (int i = 0; i < phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;
        const ElfW(Phdr) *p = &ph[i];

        int prot = 0;
        if (p->p_flags & PF_R) prot |= PROT_READ;
        if (p->p_flags & PF_W) prot |= PROT_WRITE;
        if (p->p_flags & PF_X) prot |= PROT_EXEC;

        addr_t seg_start = ALIGN_DOWN(p->p_vaddr + load_bias);
        addr_t seg_end   = ALIGN_UP  (p->p_vaddr + load_bias + p->p_memsz);
        addr_t file_end  = ALIGN_UP  (p->p_vaddr + load_bias + p->p_filesz);

        for (addr_t va = seg_start; va < seg_end; va += PAGE_SIZE) {
            size_t idx = (size_t)(va - range_start) / PAGE_SIZE;
            pg_prot[idx] |= (uint8_t)prot;
            if (va < file_end && !pg_file[idx]) {
                pg_file[idx] = 1;
                pg_off[idx]  = (uint64_t)ALIGN_DOWN(p->p_offset)
                             + (va - ALIGN_DOWN(p->p_vaddr + load_bias));
            }
        }
    }

    /* Pass 2: map file-backed runs and BSS runs (one mmap per run) */
    int flags = MAP_PRIVATE | MAP_FIXED;
    if (ehdr->e_type == ET_EXEC)
        flags |= MAP_FIXED_NOREPLACE;

    for (size_t i = 0; i < npages; ) {
        if (pg_prot[i] == 0) {          /* hole between segments */
            i++;
            continue;
        }
        void *addr = (void *)(uintptr_t)(range_start + (addr_t)(i * PAGE_SIZE));
        size_t j;
        void *m;

        if (pg_file[i]) {
            for (j = i; j < npages && pg_file[j]
                     && pg_off[j] == pg_off[i] + (j - i) * PAGE_SIZE; j++)
                ;
            m = mmap(addr, (j - i) * PAGE_SIZE, PROT_READ | PROT_WRITE,
                     flags, fd, (off_t)pg_off[i]);
        } else {
            for (j = i; j < npages && pg_prot[j] != 0 && !pg_file[j]; j++)
                ;
            m = mmap(addr, (j - i) * PAGE_SIZE, PROT_READ | PROT_WRITE,
                     flags | MAP_ANONYMOUS, -1, 0);
        }

        if (m == MAP_FAILED) {
            fprintf(stderr, "[loader] mmap 0x%zx-0x%zx: %s\n",
                    (size_t)(uintptr_t)addr,
                    (size_t)((uintptr_t)addr + (j - i) * PAGE_SIZE),
                    strerror(errno));
            goto fail_mapped;
        }
        if (record_range(&mapped, &mapped_count, &mapped_cap,
                         m, (j - i) * PAGE_SIZE) != 0) {
            munmap(m, (j - i) * PAGE_SIZE);
            fprintf(stderr, "[loader] out of memory\n");
            goto fail_mapped;
        }
        i = j;
    }

    /* Zero the BSS tail of file-backed segments (the full BSS pages are
     * already zero from the anonymous mapping above). */
    for (int i = 0; i < phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;
        const ElfW(Phdr) *p = &ph[i];
        addr_t file_limit = p->p_vaddr + load_bias + p->p_filesz;
        addr_t seg_end    = ALIGN_UP(p->p_vaddr + load_bias + p->p_memsz);
        if (file_limit < seg_end) {
            addr_t tail_end = ALIGN_UP(file_limit);
            if (tail_end > seg_end)
                tail_end = seg_end;
            memset((void *)(uintptr_t)file_limit, 0, tail_end - file_limit);
        }
    }

    /* Pass 3: apply final protections (coalesce equal runs) */
    for (size_t i = 0; i < npages; ) {
        int prot = pg_prot[i];
        size_t j = i;
        while (j < npages && pg_prot[j] == (uint8_t)prot)
            j++;
        if (prot != 0 && mprotect((void *)(uintptr_t)(range_start + (addr_t)(i * PAGE_SIZE)),
                                  (j - i) * PAGE_SIZE, prot) != 0) {
            fprintf(stderr, "[loader] mprotect 0x%zx-0x%zx: %s\n",
                    (size_t)(range_start + (addr_t)(i * PAGE_SIZE)),
                    (size_t)(range_start + (addr_t)(j * PAGE_SIZE)),
                    strerror(errno));
            goto fail_mapped;
        }
        i = j;
    }

    /* locate the loaded phdr */
    addr_t phdr_addr = 0;
    for (int i = 0; i < phnum; i++) {
        if (ph[i].p_type == PT_PHDR) {
            phdr_addr = load_bias + ph[i].p_vaddr;
            break;
        }
    }
    if (!phdr_addr) {
        for (int i = 0; i < phnum; i++) {
            if (ph[i].p_type == PT_LOAD && ph[i].p_offset == 0) {
                ElfW(Ehdr) *le = (ElfW(Ehdr) *)(uintptr_t)(load_bias + ph[i].p_vaddr);
                phdr_addr = (addr_t)(uintptr_t)le + le->e_phoff;
                break;
            }
        }
    }

    free(pg_prot);
    free(pg_file);
    free(pg_off);
    free(mapped);

    res->base      = base;
    res->entry     = ehdr->e_entry + load_bias;
    res->load_bias = load_bias;
    res->phdr_addr = phdr_addr;
    return 0;

fail_mapped:
    unmap_ranges(mapped, mapped_count);
fail_reserve:
    if (reserve)
        munmap(reserve, total_size);
    free(mapped);
    free(pg_prot);
    free(pg_file);
    free(pg_off);
    return -1;
}

/* ================================================================
 *  Stack construction
 *
 *  layout (high → low):
 *    [ strings area: random bytes (16) + execfn ]
 *    [ 16-byte alignment padding ]
 *    [ AT_NULL              ]  ← auxv end
 *    [ {type, val}  ...     ]
 *    [ {type, val}          ]  ← auxv start
 *    [ NULL (envp terminator)]
 *    [ envp[m-1] ... envp[0]]
 *    [ NULL (argv terminator)]
 *    [ argv[n-1] ... argv[0]]
 *    [ argc                 ]  ← sp
 *
 *  pointer size = sizeof(addr_t), 4 bytes on 32-bit, 8 bytes on 64-bit.
 *  each auxv entry is 2×sizeof(addr_t) bytes.
 * ================================================================ */
static addr_t build_initial_stack(addr_t sp_top,
                                  int argc, char **argv, char **envp,
                                  State *st, const char *exec_path)
{
    int envc;
    for (envc = 0; envp[envc]; envc++) {}

    uint8_t rand_bytes[16];
    if (getrandom(rand_bytes, 16, 0) != 16)
        memset(rand_bytes, 0, 16);

    /* auxv entries */
    typedef ElfW(auxv_t) auxv_t;
    enum { AV_MAX = 30 };
    auxv_t av[AV_MAX];
    int ac = 0;

#define A(t, v) av[ac++] = (auxv_t){ .a_type = (t), .a_un.a_val = (addr_t)(v) }

    A(AT_PHDR,   st->target.phdr_addr);
    A(AT_PHENT,  sizeof(ElfW(Phdr)));
    A(AT_PHNUM,  st->phnum);
    A(AT_PAGESZ, PAGE_SIZE);
    A(AT_ENTRY,  st->target.entry);
    A(AT_CLKTCK, 100);
    A(AT_UID,    getuid());
    A(AT_EUID,   geteuid());
    A(AT_GID,    getgid());
    A(AT_EGID,   getegid());
    A(AT_SECURE, 0);
    A(AT_HWCAP,  getauxval(AT_HWCAP));

    if (st->has_interp)
        A(AT_BASE, st->interp.base);

    {
        unsigned long vdso = getauxval(AT_SYSINFO_EHDR);
        if (vdso)
            A(AT_SYSINFO_EHDR, vdso);
    }
#undef A

    /* +AT_EXECFN, AT_RANDOM, AT_NULL */
    if (ac > AV_MAX - 3) {
        fprintf(stderr, "[loader] too many auxv entries\n");
        return 0;
    }

    /* compute the stack space; str_sz is the string area rounded up to a
     * 16-byte boundary, i.e. it includes the padding before the pointers */
    size_t ptr_sz  = sizeof(addr_t);
    size_t elen    = strlen(exec_path) + 1;
    size_t str_sz  = (elen + 16 + 15) & ~15UL;
    size_t data_sz = (1 + argc + 1 + envc + 1) * ptr_sz;          /* argc+argv+envp */
    size_t aux_sz  = (ac + 3) * sizeof(auxv_t);
    size_t total   = (str_sz + data_sz + aux_sz + 15) & ~15UL;

    addr_t bottom = sp_top - total;

    /* string area */
    uint8_t *p = (uint8_t *)(uintptr_t)bottom;

    char *execfn_s = (char *)p;
    memcpy(execfn_s, exec_path, elen);
    p += elen;

    uint8_t *rand_p = p;
    memcpy(rand_p, rand_bytes, 16);
    p += 16;

    /* 16-byte alignment */
    p = (uint8_t *)(((uintptr_t)p + 15) & ~(uintptr_t)15);

    /* pointer + auxv area */
    addr_t *wp = (addr_t *)p;
    addr_t *argc_ptr = wp;

    *wp++ = argc;
    for (int i = 0; i < argc; i++)  *wp++ = (addr_t)(uintptr_t)argv[i];
    *wp++ = 0;
    for (int i = 0; i < envc; i++)  *wp++ = (addr_t)(uintptr_t)envp[i];
    *wp++ = 0;

    /* auxv: fill forward (auxv is an unordered set, AT_NULL-terminated) */
    auxv_t *w = (auxv_t *)wp;
    for (int i = 0; i < ac; i++)
        *w++ = av[i];
    *w++ = (auxv_t){ .a_type = AT_EXECFN, .a_un.a_val = (addr_t)(uintptr_t)execfn_s };
    *w++ = (auxv_t){ .a_type = AT_RANDOM, .a_un.a_val = (addr_t)(uintptr_t)rand_p };
    *w++ = (auxv_t){ .a_type = AT_NULL,   .a_un.a_val = 0 };

    return (addr_t)(uintptr_t)argc_ptr;
}

/* ================================================================
 *  Jump
 * ================================================================ */
__attribute__((noreturn))
static void jump_to(addr_t entry, addr_t sp)
{
    __asm__ volatile (
        "mv   sp, %0\n\t"
        "jr   %1\n\t"
        :
        : "r"((uintptr_t)sp), "r"((uintptr_t)entry)
        : "memory"
    );
    __builtin_unreachable();
}

/* ================================================================
 *  main
 * ================================================================ */
int main(int argc, char **argv, char **envp)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf_file> [args...]\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    ElfW(Ehdr) ehdr;
    ElfW(Phdr) *ph = NULL;
    int phnum = 0;
    if (read_elf_headers(fd, &ehdr, &ph, &phnum) != 0) {
        fprintf(stderr, "unsupported ELF (need %d-bit RISC-V ET_EXEC/ET_DYN)\n",
                ELF_CLASS == ELFCLASS64 ? 64 : 32);
        close(fd);
        return 1;
    }

    State st = { .phnum = (uint16_t)phnum };

    if (load_elf(fd, &ehdr, ph, phnum, &st.target) != 0) {
        fprintf(stderr, "failed to load %s\n", path);
        free(ph);
        close(fd);
        return 1;
    }

    /* find PT_INTERP and load the dynamic linker */
    st.has_interp = 0;
    for (int i = 0; i < phnum; i++) {
        if (ph[i].p_type != PT_INTERP)
            continue;

        char ipath[256];
        if (ph[i].p_filesz <= 0 || ph[i].p_filesz >= (off_t)sizeof(ipath)) {
            fprintf(stderr, "invalid PT_INTERP path size\n");
            free(ph); close(fd); return 1;
        }

        ssize_t nread = pread(fd, ipath, ph[i].p_filesz, ph[i].p_offset);
        if (nread != (ssize_t)ph[i].p_filesz) {
            fprintf(stderr, "failed to read interpreter path\n");
            free(ph); close(fd); return 1;
        }
        ipath[ph[i].p_filesz] = '\0';

        PRINT_DEBUG("interpreter: %s", ipath);

        int fd_ld = open(ipath, O_RDONLY);
        if (fd_ld < 0) {
            fprintf(stderr, "cannot open %s: %s\n", ipath, strerror(errno));
            free(ph); close(fd); return 1;
        }

        ElfW(Ehdr) ld_ehdr;
        ElfW(Phdr) *ld_ph = NULL;
        int ld_phnum = 0;
        if (read_elf_headers(fd_ld, &ld_ehdr, &ld_ph, &ld_phnum) != 0
            || load_elf(fd_ld, &ld_ehdr, ld_ph, ld_phnum, &st.interp) != 0) {
            fprintf(stderr, "failed to load %s\n", ipath);
            free(ld_ph);
            close(fd_ld);
            free(ph);
            close(fd);
            return 1;
        }
        free(ld_ph);
        close(fd_ld);
        st.has_interp = 1;
        break;
    }
    free(ph);
    close(fd);

    /* create a new stack */
    enum { STACK_SIZE = 8UL * 1024 * 1024 };
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) { perror("mmap stack"); return 1; }

    int targc = (argc > 1) ? argc - 1 : 1;
    char **targv = malloc((targc + 1) * sizeof(char *));
    targv[0] = (char *)path;
    for (int i = 1; i < targc; i++)
        targv[i] = argv[i + 1];
    targv[targc] = NULL;

    addr_t sp = build_initial_stack((addr_t)(uintptr_t)stack + STACK_SIZE,
                                    targc, targv, envp, &st, path);
    if (!sp) {
        fprintf(stderr, "failed to build initial stack\n");
        return 1;
    }

    addr_t final_entry = st.has_interp ? st.interp.entry : st.target.entry;

    PRINT_DEBUG("target  entry:  0x%zx", (size_t)st.target.entry);
    PRINT_DEBUG("target  phdr:   0x%zx", (size_t)st.target.phdr_addr);
    if (st.has_interp)
        PRINT_DEBUG("interp  base:  0x%zx  entry: 0x%zx",
            (size_t)st.interp.base, (size_t)st.interp.entry);
    PRINT_DEBUG("sp:             0x%zx", (size_t)sp);
    PRINT_DEBUG("jumping to %s...", st.has_interp ? "ld.so" : "program entry");

    jump_to(final_entry, sp);
}
