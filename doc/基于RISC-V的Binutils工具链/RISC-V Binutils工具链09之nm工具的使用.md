# RISC-V Binutils工具链09之nm工具的使用

[TOC]

nm表示name或者name list，顾名思义，其用于列出目标文件（`.o`、`.a`）或可执行文件中的 符号表（Symbol Table），是调试、链接分析和固件开发的重要工具。

## 1 nm 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-nm --help
~~~

输出log见：[riscv64-unknown-linux-gnu-nm_help](images/riscv64-unknown-linux-gnu-nm_help.log)

### 1.1 基本语法

```bash
$ riscv64-unknown-linux-gnu-nm [选项] [文件...]
```

**使用示例：**
默认行为：读取 `a.out`，列出所有符号，按名称排序（BSD 格式）

示例：
```bash
$ riscv64-unknown-linux-gnu-nm program.o
$ riscv64-unknown-linux-gnu-nm libmylib.a
```
举个例子，分析某个.o 文件：

~~~sh
 $ riscv64-unknown-linux-gnu-nm program.o
         U atexit
00000000 T main
         U memset
         U model_weight
         U printf
         U strcmp
00000000 t __tcf_0
00000000 t __tcf_1
         U _Z11MicroPrintfPKcz
00000000 T _Z5setupv
00000000 b _ZGVZ5setupvE11op_resolver
00000000 b _ZGVZ5setupvE18static_interpreter
00000000 b _ZL10_bc_endcyc
00000000 b _ZL10_bc_sttcyc
00000000 b _ZL10_bc_sumcyc
00000000 b _ZL10_bc_usecyc
00000000 b _ZL8_bc_ercd
00000000 b _ZL9_bc_lpcnt
00000000 B _ZN5mnist11interpreterE
00000000 B _ZN5mnist5modelE
00000000 B _ZN5mnist6input0E
00000000 B _ZN5mnist6input1E
~~~

第一列表示符号地址，对于已分配地址的符号（如已定义的函数、变量），会显示其在内存中的地址，若符号尚未分配地址（如未定义的外部符号），则这一列可能显示为 `00000000` 或空白。

第二列表示符号类型说明。

第三列表示符号名称。

**符号类型说明：**
nm 输出的符号通常带一个类型字母，常见如下：

| 字符   | 含义                                        |
| ------ | ------------------------------------------- |
| -T, -t | 文本段（代码）符号<br> `T`: 全局, `t`: 局部 |
| -D, -d | 初始化数据段符号（`.data`）                 |
| -B, -b | 未初始化数据段符号（`.bss`）                |
| -U     | 未定义符号（引用自外部）                    |
| -W     | 弱符号（weak symbol）                       |
| -I     | 间接符号（indirect）                        |
| -C     | 公共符号（common symbol）                   |
| -V     | 弱对象符号（weak object）                   |
| -      | 调试符号或未知类型                          |


### 1.2 符号选择控制

控制显示哪些类型的符号。

| 选项 | 长选项           | 说明                                           |
| ---- | ---------------- | ---------------------------------------------- |
| -g   | --extern-only    | 仅显示外部（全局）符号（`T`, `D`, `U` 等）     |
| -u   | --undefined-only | 仅显示未定义符号（通常来自外部引用）           |
| -U   | --defined-only   | 仅显示已定义符号                               |
| -D   | --dynamic        | 显示动态符号表（`.dynsym`），用于共享库分析    |
| -a   | --debug-syms     | 显示调试专用符号（如 `.debug` 节中的符号）     |
|      | --special-syms   | 包含特殊符号（如调试、段相关符号）             |
|      | --synthetic      | 显示合成符号（由链接器或工具生成，非源码定义） |
| -W   | --no-weak        | 忽略弱符号（weak symbols）                     |


### 1.3 输出格式控制
控制符号的显示格式和内容。

| 选项      | 长选项          | 说明                                                         |
| --------- | --------------- | ------------------------------------------------------------ |
| -f FORMAT | --format=FORMAT | 指定输出格式：<br> • `bsd`（默认）<br> • `sysv`<br> • `posix`<br> • `just-symbols`（仅符号名） |
| -B        |                 | 等价于 `--format=bsd`                                        |
| -P        | --portability   | 等价于 `--format=posix`                                      |
| -j        | --just-symbols  | 仅输出符号名（等价于 `--format=just-symbols`）               |
| -t RADIX  | --radix=RADIX   | 指定地址进制：<br> `d`（十进制）、`o`（八进制）、`x`（十六进制）<br> 示例：`-tx` 显示十六进制地址 |


### 1.4  排序控制
控制符号的输出顺序。

| 选项 | 长选项         | 说明                                                         |
| ---- | -------------- | ------------------------------------------------------------ |
| -n   | --numeric-sort | 按地址（数值）升序排序                                       |
| -p   | --no-sort      | 不排序，按符号在文件中出现顺序输出                           |
| -r   | --reverse-sort | 逆序输出（配合 `-n` 或默认名称排序使用）<br>如：`-nr` 表示按地址降序 |

### 1.5 附加信息显示
控制是否显示额外信息。

| 选项 | 长选项         | 说明                                                         |
| ---- | -------------- | ------------------------------------------------------------ |
| -l   | --line-numbers | 使用调试信息（DWARF/STABS）显示符号对应的源文件名和行号      |
| -S   | --print-size   | 显示已定义符号的大小（字节）                                 |
| -s   | --print-armap  | 对于静态库（`.a` 文件），显示归档成员索引（即哪个 `.o` 定义了该符号） |

**查看数组使用的内存空间**：

~~~sh
$ riscv64-unknown-linux-gnu-nm -S program.o
~~~

### 1.6 文件与符号修饰

控制文件名、符号名等显示方式。

| 选项 | 长选项                                                     | 说明                                                         |
| ---- | ---------------------------------------------------------- | ------------------------------------------------------------ |
| -A   | --print-file-name                                          | 在每个符号前打印所属文件名（对多文件或库很有用）             |
| -o   |                                                            | 等价于 `-A`                                                  |
|      | --ifunc-chars=CHARS                                        | 自定义IFUNC 符号的显示字符（如 `@`、`#`）                    |
|      | --without-symbol-versions                                  | 不显示符号的版本后缀（如 `func@GLIBC_2.3` → `func`）         |
|      | --unicode={default\|show\|invalid\|hex\|escape\|highlight} | 控制如何处理 UTF-8 符号名：<br> • `hex`：非 ASCII 显示为 `\xNN`<br> • `escape`：转义控制字符 |

### 1.7 符号名处理（Demangling）
C++、Rust 等语言使用“名称修饰”（mangling），此部分控制是否还原。

| 选项 | 长选项             | 说明                                                         |
| ---- | ------------------ | ------------------------------------------------------------ |
| -C   | --demangle[=STYLE] | 反修饰符号名（还原为可读形式）<br> 可选风格：<br> • `auto`（自动）<br> • `gnu-v3`（C++）<br> • `java`, `rust`, `dlang`, `gnat`（Ada）<br> • `none`（关闭）<br> 示例：`_Z3foov` → `foo()` |
|      | --no-demangle      | 不解修饰（默认行为）                                         |
|      | --recurse-limit    | 启用解修饰递归深度限制（默认）                               |
|      | --no-recurse-limit | 禁用递归限制（可能栈溢出，慎用）                             |

对于C++版本的文件，使用-C解修饰符号名方便阅读，反修饰的概念可以参考第14篇`c++filt`工具的使用。

**反修饰符号名：**

~~~sh
$ riscv64-unknown-linux-gnu-nm -C program.o
~~~

### 1.8 帮助与版本

| 选项 | 长选项    | 说明               |
| ---- | --------- | ------------------ |
| -h   | --help    | 显示帮助信息并退出 |
| -v   | --version | 显示版本号并退出   |

## 2 典型使用场景

| 目的                     | 命令                                                      |
| ------------------------ | --------------------------------------------------------- |
| 列出所有符号（默认）     | riscv64-unknown-linux-gnu-nm file.o                       |
| 按地址排序               | riscv64-unknown-linux-gnu-nm -n file.o                    |
| 仅看未定义符号           | riscv64-unknown-linux-gnu-nm -u file.o                    |
| 显示符号大小             | riscv64-unknown-linux-gnu-nm -S file.o                    |
| 显示文件名前缀           | riscv64-unknown-linux-gnu-nm -A lib.a                     |
| C++ 符号解修饰           | riscv64-unknown-linux-gnu-nm -C file.o                    |
| 显示源码行号             | riscv64-unknown-linux-gnu-nm -l file.o（需编译时带 `-g`） |
| 查看动态符号（共享库）   | riscv64-unknown-linux-gnu-nm -D lib.so                    |
| 静态库中查找符号来源     | riscv64-unknown-linux-gnu-nm -s libmylib.a                |
| 仅输出符号名（用于脚本） | riscv64-unknown-linux-gnu-nm -j file.o                    |
| 十六进制地址输出         | riscv64-unknown-linux-gnu-nm -tx file.o                   |
