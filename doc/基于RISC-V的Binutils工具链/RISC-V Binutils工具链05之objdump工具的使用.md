# RISC-V Binutils工具链05之objdump工具的使用

[TOC]

objdump是反汇编工具，它可以对编译生成的二进制文件（如 ELF 可执行文件、目标文件 `.o`、库文件等）进行反汇编、结构解析和内容查看。

## 1 objdump 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-objdump --help
~~~

输出log见：[riscv64-unknown-linux-gnu-objdump_help](images/riscv64-unknown-linux-gnu-objdump_help.log)

### 1.1 反汇编与代码查看

| 选项 | 长选项                | 说明                                                         |
| ---- | --------------------- | ------------------------------------------------------------ |
| -d   | --disassemble         | 反汇编可执行节区（如 `.text`，这是最常用的命令）             |
| -D   | --disassemble-all     | 反汇编所有节区（包括数据节）                                 |
|      | --disassemble=\<sym\> | 从指定符号开始反汇编                                         |
| -S   | --source              | 在反汇编中混合显示源代码（需编译时带 `-g`）                  |
| -C   | --demangle\[=STYLE\]  | 反汇编时解修饰，如将C++修饰过的函数变量名解修饰<br/>STYLE可以是none， auto, gnu-v3，“java”， “gnat”， “dlang”， “rust” |

**反汇编：**

如下，在反汇编中混合显示源代码

~~~sh
$ riscv64-unknown-linux-gnu-objdump -dS helloworld.elf > helloworld.dasm
~~~

### 1.2 查看文件结构

| 选项 | 长选项            | 说明                                                    |
| ---- | ----------------- | ------------------------------------------------------- |
| -f   | --file-headers    | 显示 ELF 文件头（Machine, Entry point, Flags 等）       |
| -h   | --headers         | 显示所有节区（section）的头部信息（名称、地址、大小等） |
| -x   | --all-headers     | 显示所有头部信息（等价于 `-f -h -p`）                   |
| -a   | --archive-headers | 显示归档文件（`.a` 静态库）的成员信息                   |
| -p   | --private-headers | 显示目标格式特有的私有头部信息                          |

**查看ELF文件头：**

~~~sh
$ riscv64-unknown-linux-gnu-objdump -f helloworld.elf
~~~

输出为：

~~~log
$ riscv64-unknown-linux-gnu-objdump -f helloworld.elf

helloworld:     file format elf64-littleriscv
architecture: riscv:rv64, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x0000000000010468
~~~

使用`riscv64-unknown-linux-gnu-readelf -h `可以得到更详细的信息。

### 1.3  内容查看

| 选项 | 长选项 | 说明 |
| ---------------- | ------------------------- | ------------------------------------------------------- |
| -s | --full-contents | 显示所有节区的完整内容（十六进制 dump） |
| -Z | `-Z`, `--decompress` | 显示前先解压缩节区内容（如 `.zdebug`） |

### 1.4  符号与重定位
| 选项 | 长选项 | 说明 |
| ---------------- | ------------------------- | ------------------------------------------------------- |
| -t | --syms | 显示符号表（Symbol Table） |
| -T | --dynamic-syms | 显示动态符号表（Dynamic Symbol Table） |
| -r | --reloc | 显示重定位条目（Relocation Entries） |
| -R | --dynamic-reloc | 显示动态重定位条目（如 `.dynamic` 中的） |

### 1.5  帮助与信息类

| 选项 | 长选项 | 说明 |
| ---------------- | ------------------------- | ------------------------------------------------------- |
| -v | --version | 显示 `objdump` 版本 |
| -i | --info | 列出支持的目标格式和架构 |
| -H | --help | 显示帮助信息 |
| @\<file\> |  | 从文件读取命令行选项 |


### 1.6 可选的控制参数

这些选项用于控制输出格式、目标、范围等。

| 选项 | 长选项 | 说明 |
| ---------------- | ------------------------- | ------------------------------------------------------- |
| -M OPT |  | 传递选项给反汇编器（见下文 RISC-V 特有选项） |
| | --disassembler-color=off/on/terminal/extended | 控制反汇编颜色输出 |
| | --visualize-jumps[=color\|extended-color\|off] | 用 ASCII 艺术绘制跳转线 |
| | --insn-width=WIDTH | 每行显示 WIDTH 字节的指令 |
| | --[no-]show-raw-insn | 是否显示指令的十六进制编码 |
| | --no-addresses | 不显示地址 |
| | --prefix-addresses | 显示完整地址 |
| -z | --disassemble-zeroes | 不跳过零字节块 |
|  | -EB | 大端模式（Big Endian） |
|  | -EL | 小端模式（Little Endian，RISC-V 默认） |

**反汇编颜色控制：**

~~~sh
$ riscv64-unknown-linux-gnu-objdump -d --disassembler-color=on helloworld.elf
~~~

如果将反汇编存放到文件中，如下：

~~~sh
$ riscv64-unknown-linux-gnu-objdump -d --disassembler-color=on helloworld.elf > helloworld.dasm
~~~

查看helloworld.dasm 不能直接使用vim，需要使用：

~~~sh
# 方法1：用 cat 查看（颜色保留）
$ cat helloworld.dasm

# 方法2：用 less 查看（需 -R 参数）
$ less -R helloworld.dasm

# 方法3：用 vim + ansi 插件
$ vim helloworld.dasm
~~~

**反汇编显示跳转线：**

可以在反汇编中显示指令跳转流程，这样可以方便分析某段程序的核心循环

~~~sh
$ riscv64-unknown-linux-gnu-objdump -d --visualize-jumps=color helloworld.elf
~~~

同样的，如果将反汇编存放到文件中，需要使用上述提供的方法进行查看。


### 1.7 RISC-V 特有反汇编选项（用于 `-M`）

| 选项           | 说明                                                 |
| -------------- | ---------------------------------------------------- |
| numeric        | 使用数字寄存器名（如 `x10`），而非 ABI 名（如 `a0`） |
| no-aliases     | 仅输出标准指令，不使用伪指令（如 `mv`, `ret`）       |
| priv-spec=SPEC | 按指定特权规范显示 CSR 寄存器                        |

**RISC-V反汇编显示真实指令，而非伪指令：**

~~~sh
$ riscv64-unknown-linux-gnu-objdump -d -M no-aliases helloworld.elf > helloworld.dasm
~~~


## 2 典型使用场景

| 目的                    | 命令                                                         |
| ----------------------- | ------------------------------------------------------------ |
| 查看 ELF 头和节区信息   | riscv64-unknown-linux-gnu-objdump -x helloworld.elf          |
| 反汇编代码段            | riscv64-unknown-linux-gnu-objdump -d helloworld.elf          |
| 反汇编所有内容          | riscv64-unknown-linux-gnu-objdump -D helloworld.elf          |
| 混合显示源码与汇编      | riscv64-unknown-linux-gnu-objdump -S helloworld.elf          |
| 查看符号表              | riscv64-unknown-linux-gnu-objdump -t helloworld.elf          |
| 查看重定位信息          | riscv64-unknown-linux-gnu-objdump -r helloworld.elf          |
| 显示调试信息            | riscv64-unknown-linux-gnu-objdump -g helloworld.elf          |
| 使用 ABI 寄存器名反汇编 | riscv64-unknown-linux-gnu-objdump -d -M no-aliases helloworld.elf |
| 使用数字寄存器名反汇编  | riscv64-unknown-linux-gnu-objdump -d -M numeric helloworld.elf |
| 仅反汇编 `.text` 节     | riscv64-unknown-linux-gnu-objdump -d -j .text helloworld.elf |
