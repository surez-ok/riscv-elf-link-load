/* hello_dyn.c — dynamically-linked test program */
#include <stdio.h>
int main(int argc, char **argv) {
    printf("Hello from dynamically-linked ELF!\n");
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = %s\n", i, argv[i]);
    return 42;
}
