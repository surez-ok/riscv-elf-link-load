# RISC-V Binutils工具链12之strings工具的使用

[TOC]

strings用于从二进制文件中提取可打印字符串，是分析固件、可执行文件、内存镜像的常用工具，常用于查找硬编码路径、错误信息、配置参数等。

## 1 strings 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-strings --help
~~~

输出log见：[riscv64-unknown-linux-gnu-strings_help](images/riscv64-unknown-linux-gnu-strings_help.log)



### 1. 1 基本语法

```bash
$ riscv64-unknown-linux-gnu-strings [选项] [文件...]
```

### 1. 2 扫描范围控制
指定从文件的哪些部分提取字符串。

| 选项 | 长选项 | 说明                                                         |
| ---- | ------ | ------------------------------------------------------------ |
| -a   | --all  | 扫描整个文件内容（默认）<br> 包括代码段、数据段、资源等      |
| -d   | --data | 仅扫描数据段（`.data`, `.rodata`, `.bss` 等）<br> 更快，减少噪音，适用于已知结构的 ELF 文件 |


### 1. 2 字符串识别控制
定义“什么是字符串”。

| 选项       | 长选项                   | 说明                                                         |
| ---------- | ------------------------ | ------------------------------------------------------------ |
| -n \<num\> | --bytes=\<num\>          | 设置最小字符串长度（默认为 4）<br> 示例：`-n 8` 表示只输出 ≥8 个字符的字符串 |
| -w         | --include-all-whitespace | 将所有空白字符（如 `\t`, `\n`, `\r`）视为有效字符串字符<br> 否则仅 `SPACE` 和 `\n` 被允许 |

示例：
```bash
$ riscv64-unknown-linux-gnu-strings -n 6 firmware.bin     # 只看 6 字符以上的字符串
$ riscv64-unknown-linux-gnu-strings -w program.elf        # 允许制表符等在字符串中
```


### 1. 3  输出格式与定位信息

| 选项          | 长选项                        | 说明                                                         |
| ------------- | ----------------------------- | ------------------------------------------------------------ |
| -t {o,d,x}    | --radix={o,d,x}               | 在每行字符串前打印其文件偏移地址，进制可选：<br> • `o`: 八进制<br> • `d`: 十进制<br> • `x`: 十六进制 |
| -o            |                               | 等价于 `--radix=o`（八进制偏移）                             |
| -f            | --print-file-name             | 在每个字符串前打印文件名<br> 多文件时非常有用                |
| -s \<string\> | --output-separator=\<string\> | 设置字符串之间的分隔符（默认为换行）<br> 示例：`-s " | "`    |

示例：
```bash
$ riscv64-unknown-linux-gnu-strings -t x -f *.o          # 显示文件名 + 十六进制偏移
$ riscv64-unknown-linux-gnu-strings -s ' *** ' data.bin  # 用 *** 分隔字符串
```


### 1. 4 帮助与版本

| 选项    | 长选项    | 说明                  |
| ------- | --------- | --------------------- |
| -h      | --help    | 显示帮助信息          |
| -v， -V | --version | 显示 `strings` 版本号 |

## 2 典型使用场景

| 目的                     | 命令                                              |
| ------------------------ | ------------------------------------------------- |
| 提取默认字符串（≥4字符） | riscv64-unknown-linux-gnu-strings firmware.bin    |
| 查看数据段中的字符串     | riscv64-unknown-linux-gnu-strings -d program.elf  |
| 显示偏移地址（十六进制） | riscv64-unknown-linux-gnu-strings -t x binary.img |
| 多文件并显示文件名       | riscv64-unknown-linux-gnu-strings -f *.o          |
| 提取长字符串（≥8字符）   | riscv64-unknown-linux-gnu-strings -n 8 system.elf |

