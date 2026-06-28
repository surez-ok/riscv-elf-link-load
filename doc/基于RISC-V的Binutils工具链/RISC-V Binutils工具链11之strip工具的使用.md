# RISC-V Binutils工具链11之strip工具的使用

[TOC]

strip 用于从目标文件或可执行文件中移除符号表、调试信息、节区等冗余内容，以减小文件体积，常用于发布版本或嵌入式系统。

## 1 strip 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-strip --help
~~~

输出log见：[riscv64-unknown-linux-gnu-strip_help](images/riscv64-unknown-linux-gnu-strip_help.log)  



### 1. 基本语法

```bash
$ riscv64-unknown-linux-gnu-strip [选项] <输入文件...>
```

 默认行为：直接修改输入文件（原地 strip），除非使用 `-o`

示例：
```bash
# 原地strip，直接修改输入文件
$ riscv64-unknown-linux-gnu-strip  helloworld.elf
# 使用-o，不修改输入文件
$ riscv64-unknown-linux-gnu-strip  -o stripped.elf helloworld.elf
```

---

### 1.2 基本剥离模式
控制移除哪些类型的符号或节区。

| 选项              | 长选项           | 说明                                                         |
| ----------------- | ---------------- | ------------------------------------------------------------ |
| -s                | --strip-all      | 移除 所有符号与重定位信息<br>最彻底的剥离，仅保留执行所需内容 |
| `-g`, `-S`, `-d`, | --strip-debug    | 移除所有调试符号和调试节区（如 `.debug_*`, `.line`）<br> 保留函数/全局符号 |
| -x                | --discard-all    | 移除所有非全局符号（保留 `global` 符号）<br> 比 `-s` 温和    |
| -X                | --discard-locals | 移除编译器生成的局部符号（如 `.Ltmp`, `.LC0`）<br> 通常与调试或优化相关 |
| -R \<name\> | --remove-section=\<name\> | 从输出中移除指定节区<br> 如：`-R .comment` 删除注释节<br> 可多次使用 |
|      | --remove-relocations \<name\>        | 移除指定节区的重定位信息                                |
|      | --strip-section-headers              | 移除节区头表（Section Header Table）<br> 使文件更难反汇编分析 |


### 1.3 保留控制
指定不剥离的符号或节区。

| 选项        | 长选项                  | 说明                                                         |
| ----------- | ----------------------- | ------------------------------------------------------------ |
| -K \<name\> | --keep-symbol=\<name\>  | 保留指定符号（不删除）<br> 可多次使用                        |
| -N \<name\> | --strip-symbol=\<name\> | 移除指定符号（即使其他选项保留）<br> 优先级高于 `--keep-symbol` |
|             | --keep-section=\<name\> | 不剥离指定节区（如 `--keep-section=.rodata`）                |
|             | --keep-section-symbols  | 保留节区符号（section symbols，如 `ABS`, `UND`）             |
|             | --keep-file-symbols     | 保留文件符号（每个源文件对应的符号）                         |
|             | --only-keep-debug       | 仅保留调试信息，剥离其他所有内容<br> 用于生成独立的 `.debug` 文件 |

### 1.4 输入/输出格式控制
| 选项           | 长选项                     | 说明                                                     |
| -------------- | -------------------------- | -------------------------------------------------------- |
| -I \<bfdname\> | --input-target=\<bfdname\> | 指定输入文件的二进制格式（BFD 名称）                     |
| -O \<bfdname\> | -output-target=\<bfdname\> | 指定输出文件的格式                                       |
| -F \<bfdname\> | --target=\<bfdname\>       | 同时设置输入和输出格式                                   |
| -o \<file\>    |                            | 将剥离后的结果写入 `<file>`<br> 避免修改原文件，推荐使用 |

### 1.5 帮助

| 选项 | 长选项    | 说明                                  |
| ---- | --------- | ------------------------------------- |
| -v   | --verbose | 显示详细信息：列出所有被修改的文件    |
| -V   | --version | 显示版本号                            |
| -h   | --help    | 显示帮助信息                          |
|      | --info    | 列出当前 `strip` 支持的目标格式与架构 |

## 2 典型使用场景

| 目的                          | 命令                                                         |
| ----------------------------- | ------------------------------------------------------------ |
| 剥离调试信息（保留符号）      | riscv64-unknown-linux-gnu-strip -g helloworld.elf            |
| 彻底剥离所有符号              | riscv64-unknown-linux-gnu-strip -s helloworld.elf            |
| 保留特定符号（如 `main`）     | riscv64-unknown-linux-gnu-strip -K main helloworld.elf       |
| 保留符号并使用通配符          | riscv64-unknown-linux-gnu-strip -w -K 'uart_*' -K 'i2c_*' helloworld.elf |
| 移除特定节区（如 `.comment`） | riscv64-unknown-linux-gnu-strip -R .comment helloworld.elf   |
| 剥离并保留时间戳              | riscv64-unknown-linux-gnu-strip -p -o stripped.elf helloworld.elf |
| 处理静态库并确定性输出        | riscv64-unknown-linux-gnu-strip -D libmylib.a                |
| 查看支持的格式                | riscv64-unknown-linux-gnu-strip --info                       |
