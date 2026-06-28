// a.c
extern int foo(void);

int shared = 5;

int main() {
    int a = foo();
    return 0;
}
