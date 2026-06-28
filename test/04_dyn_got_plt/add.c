int g_sum = 0;             // 全局变量

int add(int a, int b)
{
    g_sum = a + b;         // 经 GOT 间接访问
    return g_sum;
}

