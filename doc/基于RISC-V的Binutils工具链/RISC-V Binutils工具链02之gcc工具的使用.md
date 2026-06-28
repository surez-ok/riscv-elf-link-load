# RISC-V Binutils工具链02之gcc工具的使用

[TOC]

`riscv64-unknown-linux-gnu-gcc` 是一个 交叉编译器（cross-compiler），用于在 x86_64（或其他架构）主机上编译出能在 RISC-V 位 Linux 系统上运行的程序。

## 1 gcc 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-gcc --help
~~~

输出log见：[riscv64-unknown-linux-gnu-gcc_help](./images/riscv64-unknown-linux-gnu-gcc_help.log)

注意：riscv64-unknown-linux-gnu-gcc  --help 不是列出所有选项（如果全部列出将十分冗长），只提供常用和重要选项。

### 1.1 帮助与信息类

| 选项                             | 含义                                                        |
| -------------------------------- | ----------------------------------------------------------- |
| --help                           | 显示所有可用选项的简要帮助信息                              |
| --target-help                    | 显示特定于目标的命令行选项（包括汇编器和链接器选项）        |
| --help=\{common,optimizers,...\} | 显示特定类别的帮助（如 `--help=warnings` 显示所有警告选项） |
| --version                        | 显示编译器版本                                              |
| -dumpversion                     | 仅输出编译器版本号（如 `15.1.0`）                           |
| -dumpmachine                     | 显示目标机器架构（如 `riscv64-unknown-linux-gnu`）          |

常用示例：

**显示所有警告选项：**

~~~sh
$ riscv64-unknown-linux-gnu-gcc --help=warning
The following options control compiler warning messages:
  --all-warnings              Same as -Wall.
  --extra-warnings            Same as -Wextra.
  -W                          This switch is deprecated; use -Wextra instead.  Same as
                              -Wextra.
  -WNSObject-attribute        Warn if the NSObject attribute is applied to a non-typedef.
  -Wabi                       Warn about things that will change when compiling with an ABI-
                              compliant compiler.
  -Wabi-tag                   Warn if a subobject has an abi_tag attribute that the complete
                              object type does not have.
  -Wabi=                      Warn about things that change between the current -fabi-version
                              and the specified version.
  -Wabsolute-value            Warn on suspicious calls of standard functions computing
                              absolute values.
  -Waddress                   Warn about suspicious uses of memory addresses.
  -Waddress-of-packed-member  Warn when the address of packed member of struct or union is
                              taken.
  ...
~~~

**查看-O2开了哪些优化选项：**

~~~sh
$ riscv64-unknown-linux-gnu-gcc -O2 --help=optimizers -Q
The following options control optimizations:
  --param=lto-partition-locality-cloning=       maximal
  -O<number>
  -Ofast
  -Og
  -Os
  -Oz
  -faggressive-loop-optimizations       [enabled]
  -falign-functions                     [enabled]
  -falign-functions=
  -falign-jumps                         [enabled]
  -falign-jumps=
  -falign-labels                        [enabled]
  -falign-labels=
  -falign-loops                         [enabled]
  -falign-loops=
  -fallocation-dce                      [enabled]
  -fallow-store-data-races              [disabled]
  -fassociative-math                    [disabled]
  -fassume-sane-operators-new-delete    [available in C++, ObjC++]
  -fasynchronous-unwind-tables          [enabled]
  -fauto-inc-dec                        [enabled]
...
~~~

### 1.2 路径与环境查询

| 选项                          | 说明                                            |
| ----------------------------- | ----------------------------------------------- |
| -print-search-dirs            | 显示编译器搜索头文件、库、程序的路径            |
| -print-libgcc-file-name       | 显示 `libgcc` 库的完整路径                      |
| -print-file-name=\<lib\>      | 显示指定库（如 `libc.a`）的完整路径             |
| -print-prog-name=\<prog\>     | 显示编译器组件路径（如 `as`, `ld`）             |
| -print-multiarch              | 显示目标的 GNU 三元组（如 `riscv64-linux-gnu`） |
| -print-multi-directory        | 显示 `libgcc` 的版本根目录                      |
| -print-multi-lib              | 显示多库支持的目录映射（用于不同 ABI 配置）     |
| -print-multi-os-directory     | 显示操作系统库的相对路径                        |
| -print-sysroot                | 显示 sysroot 根目录路径                         |
| -print-sysroot-headers-suffix | 显示头文件在 sysroot 中的子路径后缀             |

常用示例：

**打印编译器搜索头文件、库、程序的路径：**

~~~sh
$ riscv64-unknown-linux-gnu-gcc -print-search-dirs
~~~

这个命令打印出 `riscv64-unknown-linux-gnu-gcc`在编译、汇编和链接过程中，用于搜索程序组件（如 cc1）、头文件（include files）和库文件（libraries）的所有目录路径。

**多库支持：**

可以用来排查这个gcc是否支持传入的march与mabi，如果不支持可能要修改传入的march与mabi，或者重新添加编译gcc工具链

~~~sh
$ riscv64-unknown-linux-gnu-gcc -print-multi-lib
.;
lib32/ilp32;@march=rv32imac@mabi=ilp32
lib32/ilp32d;@march=rv32imafdc@mabi=ilp32d
lib64/lp64;@march=rv64imac@mabi=lp64
lib64/lp64d;@march=rv64imafdc@mabi=lp64d
~~~

### 1.3 控制编译流程

| 选项              | 说明                                                      |
| ----------------- | --------------------------------------------------------- |
| -E                | 仅预处理（不编译、汇编、链接）                            |
| -S                | 编译到汇编代码（不汇编、链接）                            |
| -c                | 编译并汇编，生成 `.o` 文件（不链接）                      |
| -o \<file\>       | 指定输出文件名                                            |
| -x \<language\>   | 强制指定后续文件的语言（`c`, `c++`, `assembler`, `none`） |
| -std=\<standard\> | 指定 C/C++ 语言标准（如 `c11`, `c++17`）                  |

其中的几个选项的使用可以看上一篇的编译helloworld的例子，这里就不重复举例了

### 1.4 输出与调试控制

| 选项                | 说明                                                 |
| ------------------- | ---------------------------------------------------- |
| -save-temps         | 保留中间文件（`.i`, `.s`, `.o`）                     |
| -save-temps=\<arg\> | 可指定保存方式（如 `obj`, `source`）                 |
| -pipe               | 使用管道代替临时文件，提升编译速度                   |
| -time               | 统计每个子进程（预处理、编译、汇编、链接）的执行时间 |
| -v                  | 显示编译器调用的子命令（如 `cc1`, `as`, `ld`）       |
| -###                | 类似 `-v`，但命令被引号包围且不执行（用于调试脚本）  |

**预处理、编译、汇编、链接的用户时间和系统时间：**

~~~sh
$ riscv64-unknown-linux-gnu-gcc -march=rv64imafdc -mabi=lp64d -O2 -g -time helloworld.c
-o helloworld
# cc1 0.04 0.02
# as 0.00 0.00
# collect2 0.02 0.01
~~~

### 1.5 链接与输出类型

| 选项    | 说明                                                         |
| ------- | ------------------------------------------------------------ |
| -shared | 生成共享库（`.so`）                                          |
| -pie    | 生成位置无关的可执行文件（Position Independent Executable），常用于安全加固 |

### 1.6 传递选项给子工具

| 选项                   | 说明                                                  |
| ---------------------- | ----------------------------------------------------- |
| -Wp,\<options\>        | 传递给预处理器（如 `-Wp,-DDEBUG`）                    |
| -Wa,\<options\>        | 传递给汇编器（如 `-Wa,-adhln` 输出带源码的汇编列表）  |
| -Wl,\<options\>        | 传递给链接器（如 `-Wl,-Map=output.map` 生成映射文件） |
| -Xpreprocessor \<arg\> | 单独传递一个参数给预处理器                            |
| -Xassembler \<arg\>    | 单独传递一个参数给汇编器                              |
| -Xlinker \<arg\>       | 单独传递一个参数给链接器                              |

**使用-Wl传递标记给链接器：**

如下示例，是用来优化可执行文件的大小：

* 首先使用 `-ffunction-sections -fdata-sections` 传递给编译器，用于在编译阶段，为后续的“垃圾回收”做准备，

* 然后将`-Wl,--gc-sections`传递给链接器，用于“垃圾回收”。

这样编译器与链接器配合，将没有使用的独立代码段、数据段丢弃，不链接到最终的可执行文件中。

~~~sh
$ riscv64-unknown-linux-gnu-gcc -march=rv64imafdc \
                                -mabi=lp64d \
                                -O2 -g \
                                -ffunction-sections -fdata-sections \ # 以f开头，控制编译器的具体行为和代码生成方式
                                -Wl,--gc-sections,--check-sections \  # 以Wl开头，传递标记给链接器
                                helloworld.c -o helloworld
~~~

其中：传递多个标记可以使用 `-Wl,--gc-sections,--check-sections` 或 `-Wl,--gc-sections -Wl,--check-sections` 这两种写法是等价的。

### 1.7 其他高级选项

| 选项                    | 说明                                                         |
| ----------------------- | ------------------------------------------------------------ |
| -specs=\<file\>         | 使用自定义 `specs` 文件覆盖默认编译行为（常用于嵌入式或定制工具链） |
| --sysroot=\<directory\> | 设置 sysroot 路径，用于查找头文件和库（交叉编译常用）        |
| -B \<directory\>        | 添加目录到编译器搜索路径（查找 `as`, `ld` 等组件）           |
| -foffload=\<targets\>   | 指定卸载编译目标（用于异构计算，如 GPU）                     |
| -pass-exit-codes        | 返回所有编译阶段中的最高错误码（用于脚本判断）               |
| -dumpspecs              | 输出编译器内置的所有 `specs` 字符串（调试用）                |

### 1.8自动转发的选项前缀

以下前缀的选项会自动传递给子进程（无需 `-W`）

| 选项    | 说明                                   |
| ------- | -------------------------------------- |
| -g      | 调试信息                               |
| -f      | 编译器功能（如 `-fPIC`）               |
| -m      | 目标架构相关（如 `-march=rv64imafdc`） |
| -O      | 优化级别（如 `-O2`）                   |
| -W      | （Warning）警告控制（如 `-Wall`）      |
| --param | 内部参数调整                           |

**-f 参数与-W参数的区别：**

`-W` 与  `-f`参数可以看作GCC 编译器的两个不同“控制面板：

*   以 `-W` 开头，如`-Wall -Wextra -Werror`等，用于控制编译器发出的“警告”（Warnings），是关于“提醒”和“代码质量”，帮助开发者发现代码中的潜在错误、不良习惯或可移植性问题。
*   以 `-f` 开头，如 `-O2`, `-ftree-vectorize` 等，控制编译器的具体行为和代码生成方式。这些选项开启或关闭特定的优化、代码生成策略、语言特性、诊断行为等。

关闭方式：

* 对于 `-f...` 选项：使用 `-fno-...`来关闭

* 对于 `-W...` 选项： 使用 `-Wno-...`来关闭

## 2 典型使用场景

除了前面的基础编译与链接，还有其它的一些示例：

```sh
# 1. 使用 sysroot 和自定义头文件路径
$ riscv64-unknown-linux-gnu-gcc \
-march=rv64imafdc -mabi=lp64d \
--sysroot=/path/to/sysroot    \
-I/path/to/include            \ # 自定义头文件
-L/path/to/libs               \ # 自定义库路径
-lmylib                       \ # 链接libmylib.a/so 库
helloworld.c -o helloworld.elf

# 2. 查看链接器 map 文件
$ riscv64-unknown-linux-gnu-gcc \
-march=rv64imafdc -mabi=lp64d \
helloworld.o \
-Wl,-Map=helloworld.map \
-o helloworld.elf

# 3. 显示详细调用命令（调试工具链问题）
$ riscv64-unknown-linux-gnu-gcc \
-march=rv64imafdc -mabi=lp64d \
-v helloworld.c -o helloworld.elf
```

gcc 的有许多不同的参数和使用方式，这里就不再列举了。

**参考：**

1. [RISC-V Options (Using the GNU Compiler Collection (GCC))](https://gcc.gnu.org/onlinedocs/)

