# RISC-V Binutils工具链08之as工具的使用

[TOC]

as 是 GNU 汇编器（GNU Assembler），用于将 `.s` 或 `.S` 汇编源文件编译为 `.o` 目标文件。

## 1 as 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-as --help
~~~

输出log见：[riscv64-unknown-linux-gnu-as_help](images/riscv64-unknown-linux-gnu-as_help.log)

### 1.1 基本语法

```bash
$ riscv64-unknown-linux-gnu-as [选项] [汇编文件...]
```
**使用示例：**

```bash
$ riscv64-unknown-linux-gnu-as helloworld.s -o helloworld.o
```

**使用as与使用gcc -c进行汇编的区别：**

在 RISC-V 开发中，`riscv64-unknown-linux-gnu-as` 和 `riscv64-unknown-linux-gnu-gcc -c` 都可以将汇编文件（`.s` 或 `.S`）编译为目标文件（`.o`），但两者有如下区别：

| **特性**         | `riscv64-unknown-linux-gnu-as` (纯汇编器) | `riscv64-unknown-linux-gnu-gcc -c` (GCC 驱动)                |
| :--------------- | :---------------------------------------- | :----------------------------------------------------------- |
| **工具本质**     | GNU Binutils 中的独立汇编器               | GCC 前端，内部调用 `as` + 其他处理                           |
| **预处理**       | 不支持（需手动 `cpp` 预处理）             | 自动预处理（若文件扩展名为 `.S` 或 `-x assembler-with-cpp`） |
| **宏和条件编译** | 不支持                                    | 支持 `#define`、`#ifdef` 等                                  |
| **头文件包含**   | 不支持                                    | 支持 `#include`                                              |
| **调试信息**     | 需手动添加 `.debug` 指令                  | 自动支持（`-g` 选项）                                        |
| **优化选项**     | 不支持                                    | 支持（`-O2` 等优化标志影响汇编生成）                         |
| **默认输出**     | 无符号表/重定位信息（除非手动指定）       | 包含完整符号表和重定位信息                                   |
| **典型用途**     | 编写裸机汇编或高度控制的目标文件          | 混合 C/汇编开发或需预处理/调试的场景                         |

### 1.2 输入/输出控制

| 选项       | 说明                                       |
| ---------- | ------------------------------------------ |
| -o OBJFILE | 指定输出目标文件名（默认 `a.out`）         |
| @FILE      | 从文件 `FILE` 中读取命令行选项（每行一个） |

### 1.3 调试相关

控制是否生成以及生成何种格式的调试信息。

| 选项                       | 说明                                                         |
| -------------------------- | ------------------------------------------------------------ |
| -g, --gen-debug        | 生成调试信息（启用 DWARF 或 STABS）         |
| -D            | 生成汇编器内部调试消息（开发者用）       |
| --dump-config | 显示汇编器配置并退出                     |
| --statistics  | 打印执行过程中的统计信息（内存、时间等） |
| --version     | 显示版本号并退出                         |
| --help        | 显示所有选项帮助                         |
| --target-help | 显示目标平台（RISC-V）特定选项           |

### 1.4 警告与错误控制

控制警告、错误和信息输出级别。

| 选项             | 说明                                   |
| ---------------- | -------------------------------------- |
| -W, --no-warn    | 抑制所有警告                           |
| --warn           | 启用警告（默认）                       |
| --fatal-warnings | 将所有警告视为错误                     |
| --no-info        | 抑制信息性消息                         |
| --info           | 启用信息性消息（默认）                 |
| -Z               | 即使出现错误也生成目标文件（用于调试） |


### 1.5 RISC-V 架构专用选项

**架构与 ABI 设置：**

| 选项            | 说明                                    |
| --------------- | --------------------------------------- |
| -march=ISA      | 指定 RISC-V 架构（如 `rv64imafdc`）     |
| -mabi=ABI       | 指定 ABI（如 `lp64`, `lp64f`, `lp64d`） |
| -mbig-endian    | 生成大端（Big-Endian）代码              |
| -mlittle-endian | 生成小端（Little-Endian）代码（默认）   |

**ISA 与特权规范版本：**

| 选项                   | 说明                                                  |
| ---------------------- | ----------------------------------------------------- |
| -misa-spec=ISAspec   | 指定 ISA 规范版本：<br> `2.2`, `20190608`, `20191213` |
| -mpriv-spec=PRIVspec | 指定特权架构版本：<br> `1.10`, `1.11`, `1.12`         |

**代码生成与链接优化：**

| 选项              | 说明                                          |
| ----------------- | --------------------------------------------- |
| -fpic / -fPIC | 生成位置无关代码（PIC）                       |
| -fno-pic        | 不生成 PIC（默认）                            |
| -mrelax         | 启用链接时放松优化（如短跳转→长跳转）（默认） |
| -mno-relax      | 禁用放松优化                                  |

**属性与检查：**

| 选项           | 说明                                                   |
| -------------- | ------------------------------------------------------ |
| -march-attr    | 生成 RISC-V 架构属性节（`.note.gnu.riscv.attributes`） |
| -mno-arch-attr | 不生成架构属性                                         |
| -mcsr-check    | 启用 CSR 寄存器与特权版本检查                          |
| -mno-csr-check | 禁用 CSR 检查（默认）                                  |

## 2 典型使用场景

| 目的                 | 命令                                                         |
| -------------------- | ------------------------------------------------------------ |
| 汇编并输出目标文件   | riscv64-unknown-linux-gnu-as -o main.o main.s                |
| 保留局部符号         | riscv64-unknown-linux-gnu-as -L -o main.o main.s             |
| 生成依赖文件         | riscv64-unknown-linux-gnu-as --MD main.d -o main.o main.s    |
| 定义符号用于条件汇编 | riscv64-unknown-linux-gnu-as --defsym DEBUG=1 -o main.o main.s |
| 生成位置无关代码     | riscv64-unknown-linux-gnu-as -fpic -o main.o main.s          |
| RISC-V 64位浮点 ABI  | riscv64-unknown-linux-gnu-as -march=rv64imafdc -mabi=lp64f -o main.o main.s |
| 禁用放松优化         | riscv64-unknown-linux-gnu-as -mno-relax -o main.o main.s     |

