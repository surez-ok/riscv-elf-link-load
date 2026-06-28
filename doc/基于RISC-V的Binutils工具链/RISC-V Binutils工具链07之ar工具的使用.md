# RISC-V Binutils工具链07之ar工具的使用

[TOC]

ar（archiver）用于创建、修改和提取静态库（`.a` 文件），是构建 C/C++ 项目时链接静态库的核心工具。

注意：动态库并不是由`ar`工具创建的，2.2节举了制作静态库与动态库的示例。

## 1 ar 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-ar --help
~~~

输出log见：[riscv64-unknown-linux-gnu-ar_help](images/riscv64-unknown-linux-gnu-ar_help.log)

### 1.1 ar 常用选项

```bash
$ riscv64-unknown-linux-gnu-ar [emulation-options] [-]{command}[modifiers] [--plugin <name>] [member-name] [count] archive-file [file...]
```

| 选项               | 说明                                                         |
| ------------------ | ------------------------------------------------------------ |
| -d                 | 从归档中删除指定的成员文件                                   |
| -m[ab]             | 移动成员文件：<br> `ma`：移到某成员之后<br> `mb` 或 `mi`：移到某成员之前 |
| -p                 | 打印成员内容到标准输出（类似 `cat`）                         |
| -q[f]              | 快速追加 文件到归档末尾（不检查是否存在）<br> `qf`：截断文件名（兼容旧格式） |
| -r\[ab\]\[f\]\[u\] | 替换或插入文件：<br> 若存在则替换，否则插入<br> `ra`：插入到指定成员之后<br> `rb`/`ri`：插入到之前<br> `rf`：截断文件名<br> `ru`：仅当新文件更新时才替换 |
| -s                 | 强制生成符号表索引（等价于使用 `ranlib` 工具）<br/>推荐与 `r` 连用：`rcs` |
| -t\[O\]\[v\]       | 列出归档内容<br> `tO`：显示文件在归档中的偏移<br> `tv`：详细模式（含权限、时间、大小） |
| -x\[o\]            | 提取成员文件<br> `xo`：保留原始时间戳                        |

**基本用法：**

```bash
$ riscv64-unknown-linux-gnu-ar -rcs libmylib.a file1.o file2.o    # 创建带索引的静态库
$ riscv64-unknown-linux-gnu-ar -t libmylib.a                      # 列出库中成员
$ riscv64-unknown-linux-gnu-ar -x libmylib.a                      # 提取所有对象文件
```


### 1.2 通用修饰符（Global Options）

这些选项适用于所有命令。

| 修饰符                    | 说明                                   |
| ------------------------- | -------------------------------------- |
| -c                        | 静默创建(不警告归档不存在)             |
| -S                        | 不生成符号表（跳过索引构建，加快操作） |
| -T                        | 已弃用，请使用 `--thin`                |
| -v                        | 显示详细操作过程                       |
| -V                        | 版本信息                               |
| --record-libdeps=\<text\> | 同 `l`，指定库依赖（如 `libc m`）      |

### 1.3 高级功能与特殊选项

| 选项              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| --thin            | 创建 瘦归档（Thin Archive）：<br>成员是符号链接而非复制，节省空间，适合开发环境 |
| --target=BFDNAME  | 指定输入/输出文件的目标格式（如 `elf64-littleriscv`）        |
| --output=DIRNAME  | 提取文件时指定输出目录（用于 `x` 命令）                      |
| @\<file\>         | 从文件读取命令行参数（每行一个参数）                         |
| --plugin \<name\> | 加载指定插件（用于扩展功能，如 LTO）                         |

## 2 常用命令示例

### 2.1 ar典型使用场景

| 场景                       | 推荐用法                                                     |
| -------------------------- | ------------------------------------------------------------ |
| 构建静态库                 | riscv64-unknown-linux-gnu-ar rcs libxxx.a *.o（`c` 静默创建，`s` 生成索引） |
| 增量更新                   | riscv64-unknown-linux-gnu-ar ru libxxx.a new.o（仅更新更新的文件） |
| 可重现构建                 | 使用 `D` 修饰符：riscv64-unknown-linux-gnu-ar rcsD libxxx.a *.o |
| 开发调试                   | 使用 `--thin` 创建瘦库，节省磁盘空间                         |
| 提取调试，避免污染当前目录 | riscv64-unknown-linux-gnu-ar x --output=debug/ libxxx.a      |

### 2.2 怎么制作一个静态库

**制作静态库：**

如下命令用来制作静态库，先收集所有 .o 文件，带有路径，再一次性打包

~~~sh
$ find . -name "*.o" -print0 | xargs -0 riscv64-unknown-linux-gnu-ar rcs liboutput.a
~~~

静态库制作过程与编译过程无关，所以可以通过`ar`工具直接打包。而制作动态库不能使用`ar`工具，需要编译器与链接器参与。

**制作动态库：**

1. 编译生成位置无关的目标文件（.o）

使用 RISC-V 交叉编译器，并在编译时加上 `-fPIC` 选项，`-fPIC`：指示编译器生成位置无关代码。这是制作动态库最核心的要求，它确保代码内部不依赖绝对内存地址，从而支持在运行时被重定位。

```sh
$ riscv64-unknown-linux-gnu-gcc -fPIC -c mylib.c -o mylib.o
```

2. 链接生成动态库（.so）

使用 `-shared` 选项将目标文件链接成动态库。按照 Linux 规范，动态库文件名必须以 `lib` 开头，以 `.so` 结尾：

```shell
$ riscv64-unknown-linux-gnu-gcc -shared -o libmylib.so mylib.o
```
