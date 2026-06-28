/* hello_static.c — statically-linked test program (uses _exit to avoid exit cleanup) */
#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv) {
    printf("Hello from statically-linked ELF!\n");
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = %s\n", i, argv[i]);
    _exit(99);
}
