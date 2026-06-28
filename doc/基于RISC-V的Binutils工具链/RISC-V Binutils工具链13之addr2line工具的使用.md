# RISC-V Binutils工具链13之addr2line工具的使用
[TOC]

addr2line 用于将程序地址转换为源代码文件名和行号，帮助开发者根据二进制文件中的地址定位到源代码中的具体位置。

## 1 addr2line 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-addr2line --help
~~~

输出log见：[riscv64-unknown-linux-gnu-add2line_help](images/riscv64-unknown-linux-gnu-addr2line_help.log)

addr2line 工具常用选项如下：

| 选项 | 完整选项             | 含义                                                         |
| ---- | -------------------- | ------------------------------------------------------------ |
| -a   | --addresses          | 显示原始地址<br/>输出格式：`地址\n文件:行号`                 |
| -b   | --target=\<bfdname\> | 指定输入文件的二进制格式（BFD 名称）<br/> 如：`elf64-littleriscv` |
| -e   | --exe=\<executable\> | 指定输入的可执行文件或目标文件（含调试信息）<br/>必须包含 DWARF 调试信息（编译时加 `-g`） |
| -i   | --inlines            | 展开内联函数（inlined functions）<br/> 如果地址属于被内联的函数，会输出多行：<br/>主调函数 → 内联函数1 → 内联函数2 |
| -j   | --section=\<name\>   | 将输入的地址解释为 节区相对偏移（section-relative offset）<br/> 而非虚拟地址（VMA）<br/> 示例：`-j .text` 表示地址相对于 `.text` 段起始 |
| -p   | --pretty-print       | 美化输出，将地址、函数、文件、行号合并为一行<br/> 格式：`<address>: <function> at <file>:<line>`<br/> 更适合日志分析 |
| -s   | --basenames          | 只显示源文件名，不显示完整路径<br/> 如：`main.c` 而非 `/home/user/project/src/main.c` |
| -f   | --functions          | 显示函数名<br/> 输出格式：`函数名\n文件:行号`                |
| -C   | --demangle\[=style\] | 解修饰 C++/Rust 等语言的函数名<br/> 将 `_Z3fooi` 还原为 `foo(int)`<br/> 可选风格：`auto`, `gnu-v3`, `rust`, `dlang` 等（默认 `auto`） |
| -R   | --recurse-limit      | 启用解修饰时的递归深度限制（默认）<br/> 防止栈溢出           |
| -r   | --no-recurse-limit   | 禁用递归限制（不推荐，可能崩溃）                             |
| -h   | --help               | 显示帮助信息                                                 |
| -v   | --version            | 显示版本号                                                   |

## 2 典型使用场景

**注意：**编译elf一定要加上-g，通过readelf -S 查看可以看到 \*debug 等字段

### 2.1 addr2line基本使用方法：

~~~sh
$ riscv64-unknown-elf-addr2line -e helloworld.elf 0x1045e
~~~

-e参数后跟的是可执行文件名，0x104fc为你想要查询的地址

可能有如下输出：

~~~log
helloworld/helloworld.c:8
~~~

表示：0x1045e 对应于 helloworld/helloworld.c 的第8行

### 2.2 更多功能

**解码多个地址**

可以直接列出多个地址让 `addr2line` 一次性处理

~~~sh
$ riscv64-unknown-elf-addr2line -e helloworld.elf 0x10450 0x1045e
~~~

可能有如下输出：

~~~log
6helloworld/helloworld.c:6
helloworld/helloworld.c:8
~~~

**显示函数名：**

~~~sh
$ riscv64-unknown-elf-addr2line -e helloworld.elf -f 0x1045e
~~~

可能有如下输出：

~~~log
main
helloworld/helloworld.c:8
~~~

## 3 注意事项

1. 要使 `addr2line` 正常工作，编译时添加 `-g` 标志，必须确保使用的可执行文件或共享库包含调试信息，如果没有调试信息，`addr2line` 将无法准确地将地址映射回源代码位置。

2. 当代码编译经过深度优化，如编译器优化可能导致代码重组、内联等，得到的行号可能不完全精确。

**参考：**

1. [(49 封私信 / 80 条消息) 手把手教你用 addr2line 去 debug - 知乎](https://zhuanlan.zhihu.com/p/476554675)