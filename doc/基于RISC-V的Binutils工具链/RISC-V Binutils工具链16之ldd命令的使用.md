# RISC-V Binutils工具链18之ldd命令的使用

[TOC]

ldd（List Dynamic Dependencies）命令用于检查可执行程序依赖哪些共享库。它通过模拟动态链接器（如 `ld-linux.so`）的行为，打印出程序运行时需要加载的所有共享库及其路径。

## 1 ldd 工具常用选项

~~~sh
$ ldd --help
~~~

输出log见：[ldd_help](images/ldd_help.log)

### 1.1 基本语法

~~~sh
$ ldd <可执行文件或库...>
~~~

**检查x86上可执行程序的依赖的库：**

注意这里是在x86机器上执行ldd命令：

```bash
$ ldd /bin/ls
```

输出示例：

~~~log
# ldd /bin/ls
  linux-vdso.so.1 (0x00007ffe7214b000)
  libselinux.so.1 => /lib/x86_64-linux-gnu/libselinux.so.1 (0x00007f8c582bb000)
  libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f8c58092000)
  libpcre2-8.so.0 => /lib/x86_64-linux-gnu/libpcre2-8.so.0 (0x00007f8c57ffb000)
  /lib64/ld-linux-x86-64.so.2 (0x00007f8c58311000)
~~~

**检查交叉编译的可执行程序的依赖库：**

由于交叉编译的可执行文件并不能再x86宿主机上执行，所以不能直接使用宿主机上的ldd命令，可以用如下方式：

~~~sh
# 使用 readelf 查看依赖（不执行程序）
$ riscv64-unknown-linux-gnu-readelf -d helloworld.elf | grep NEEDED
0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 
# 或使用 objdump
$ riscv64-unknown-linux-gnu-objdump -p helloworld.elf | grep NEEDED
 NEEDED               libc.so.6
~~~

### 1.2 依赖分析与重定位处理

这些选项用于更深入地检查 ELF 文件依赖分析与重定位处理。

| 选项 | 长选项            | 说明                                                         |
| ---- | ----------------- | ------------------------------------------------------------ |
| -d   | --data-relocs     | 处理并显示数据重定位（data relocations）<br>检查全局变量、GOT 等是否正确解析 |
| -r   | --function-relocs | 处理并显示数据和函数重定位（包括函数符号）<br>检查未定义函数是否能正确解析 |

 注意：不要对不可信的二进制文件使用 `ldd -d` 或 `ldd -r`，因为这些选项会实际执行目标程序（在 `RTLD_TRACE` 模式下），可能导致恶意代码执行。可以使用`readelf -d` 或 `objdump -p`安全替代，见上述举例。

### 1.3 依赖关系报告

| 选项 | 长选项   | 说明                                                         |
| ---- | -------- | ------------------------------------------------------------ |
| -u   | --unused | 显示未被使用的直接依赖项（unused direct dependencies）<br>帮助发现冗余的 -l 链接项 |


### 1.4 帮助与版本

| 选项 | 长选项    | 说明                                                         |
| ---- | --------- | ------------------------------------------------------------ |
|      | --help    | 打印帮助信息并退出                                           |
|      | --version | 打印 `ldd` 版本信息并退出                                    |
| -v   | --verbose | 显示所有详细信息，包括：<br>• 依赖库版本<br>• 符号版本信息<br>• 内存加载地址等 |


## 2 典型使用场景

| 场景               | 命令                                   |
| ------------------ | -------------------------------------- |
| 查看程序依赖库     | ldd myprogram                          |
| 检查是否有缺失依赖 | ldd myprogram → 查看是否有 `not found` |
| 调试符号解析问题   | ldd -r myprogram                       |
| 优化链接项         | ldd -u myprogram→ 移除未使用的 `-lxxx` |
| 获取详细版本信息   | ldd -v myprogram                       |
