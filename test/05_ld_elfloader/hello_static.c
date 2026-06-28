/*
 * hello_static.c — freestanding static ET_EXEC for the loader's static path.
 *
 * Deliberately no libc: the loader calls main() directly (it does not run
 * _start / __libc_start_main), so a glibc-based static binary would lack TLS
 * setup and crash in malloc/stdio.  A raw write(2) syscall avoids that and
 * works unchanged on both RV32 and RV64.
 */
#define SYS_WRITE 64

static long syscall3(long n, long a, long b, long c)
{
    register long a7 __asm__("a7") = n;
    register long a0 __asm__("a0") = a;
    register long a1 __asm__("a1") = b;
    register long a2 __asm__("a2") = c;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2) : "memory");
    return a0;
}

int main(void)
{
    const char msg[] = "hello world!\n";
    syscall3(SYS_WRITE, 1, (long)msg, sizeof(msg) - 1);
    return 0;
}
