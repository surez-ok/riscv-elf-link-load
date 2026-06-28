#ifndef _ELF_LOADER_H_
#define _ELF_LOADER_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <elf.h>
#include "wheelc/wheelc.h"

#if defined(__LP64__)
#define ElfW(what) Elf64_ ## what
#define ELFW(what) ELF64_ ## what
#else
#define ElfW(what) Elf32_ ## what
#define ELFW(what) ELF32_ ## what
#endif

#define FLAG_LINKED     0x00000001
#define FLAG_GNU_HASH   0x00000002

#define ELF_MODULE_NAME_LEN 128

struct elf_module {
    char name[ELF_MODULE_NAME_LEN];

    ElfW(Addr) entry;
    ElfW(Addr) base;
    size_t size;
    ElfW(Addr) load_bias;
    ElfW(Half) type;

    ElfW(Phdr) *phdr;
    size_t phnum;
    ElfW(Dyn) *dynamic;
    void *elf_data;
    size_t elf_len;
    const char *strtab;
    size_t strtab_size;
    ElfW(Sym) *symtab;

    /* ELF hash*/
    size_t nbucket;
    size_t nchain;
    uint32_t *bucket;
    uint32_t *chain;

    /* GUN Hash */
    size_t gnu_nbucket;
    uint32_t *gnu_bucket;
    uint32_t *gnu_chain;
    uint32_t gnu_maskwords;
    uint32_t gnu_shift2;
    ElfW(Addr) *gnu_bloom_filter;

    /* RISC-V uses RELA type relocations */
    ElfW(Rela) *plt_rela;
    size_t plt_rela_count;
    ElfW(Rela) *rela;
    size_t rela_count;

    /* Static ELF: separate .rela.dyn and .rela.plt from section headers */
    ElfW(Rela) *static_rela_dyn;
    size_t static_rela_dyn_count;
    ElfW(Rela) *static_rela_plt;
    size_t static_rela_plt_count;

    /* Static ELF: constructor / destructor arrays */
    ElfW(Addr) *preinit_array;
    size_t preinit_array_count;
    ElfW(Addr) *init_array;
    size_t init_array_count;
    ElfW(Addr) *fini_array;
    size_t fini_array_count;

    size_t needed_count;
    size_t symtab_count;
    uint32_t flags;
    size_t refcnt;

    /* dlopen'd dependency handles for cleanup */
    void **dl_handles;
    size_t dl_handles_count;

    struct list_head list;
};

struct elf_module *load_elf_module(const char *name, const void *elf_data, size_t elf_len);
void unload_elf_module(const char *name);
/* For ET_EXEC modules, the loader resolves main() and invokes it directly. */
int run_elf_module(struct elf_module *m, const char *func);

#endif /* _ELF_LOADER_H_ */