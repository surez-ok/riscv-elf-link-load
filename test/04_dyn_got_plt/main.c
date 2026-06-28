#include <stdio.h>

extern int add(int, int);
extern int sub(int, int);

extern int g_sum, g_sub;
int main(void)
{
    add(1, 2);
    add(3, 4);
    sub(5, 6);
    sub(7, 8);
    printf("g_sum = %d, g_sub = %d\r\n", g_sum, g_sub);   // 跨模块读全局变量

    g_sum = 100;                                        // 跨模块写全局变量
    g_sub = 200;                                        // 跨模块写全局变量
    printf("g_sum = %d, g_sub = %d\r\n", g_sum, g_sub);   // 再读验证写入结果

    return 0;
}
