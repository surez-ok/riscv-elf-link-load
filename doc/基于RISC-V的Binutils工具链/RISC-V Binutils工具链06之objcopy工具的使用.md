# RISC-V Binutils工具链06之objcopy工具的使用

[TOC]

objcopy 用于复制二进制文件，并可进行格式转换、符号处理、节区修改等操作。

## 1 objcopy 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-objcopy --help
~~~

输出log见：[riscv64-unknown-linux-gnu-objcopy_help](images/riscv64-unknown-linux-gnu-objcopy_help.log)

###  1.1 基本语法

```bash
$ riscv64-unknown-linux-gnu-objcopy [选项] 输入文件 [输出文件]
```

### 1.2 输入/输出控制

控制输入和输出的文件格式与架构。

| 选项 | 长选项                         | 说明                                           |
| ---- | ------------------------------ | ---------------------------------------------- |
| -I   | --input-target \<bfdname\>     | 指定输入文件格式（如 `elf64-littleriscv`）     |
| -O   | --output-target \<bfdname\>    | 指定输出文件格式                               |
| -F   | --target \<bfdname\>           | 同时设置输入和输出格式                         |
| -B   | --binary-architecture \<arch\> | 当输入无架构信息时，指定目标架构（如 `riscv`） |

**-O 支持哪些格式？**

可以使用如下命令来查看支持哪些目标文件格式：

`objcopy` 和 `objdump` 使用相同的 BFD（Binary File Descriptor）库，因此支持的格式基本一致，所以可以通过`objdump -i`来查看。

~~~sh
$ riscv64-unknown-linux-gnu-objdump -i
~~~

以下是 `riscv64-unknown-linux-gnu-objcopy` 通常支持的主要格式：

| 格式（-O 后的名称）                    | 说明                                                         |
| -------------------------------------- | ------------------------------------------------------------ |
| binary                                 | 原始二进制镜像（最常用）<br>去除所有 ELF 头、符号表，只保留可加载段的内容。<br>常用于裸机、Bootloader、Firmware。 |
| elf32-littleriscv 或 elf64-littleriscv | 标准的ELF 格式（默认）<br>用于 Linux 可执行文件、共享库等。  |
| ihex                                   | Intel HEX 格式<br>ASCII 文本格式，常用于烧录到 Flash、MCU。  |
| srec 或 s19                            | Motorola S-Record 格式<br>另一种文本格式，用于嵌入式烧录。   |
| verilog                                | Verilog 数据文件格式<br>将二进制内容转为 Verilog 初始化数组，用于 FPGA ROM 初始化。 |

**为什么 -O binary 文件比-O ihex 大？**

在嵌入式开发中，`riscv64-unknown-linux-gnu-objcopy` 的 `-O binary` 和 `-O ihex` 选项用于将 ELF 文件转换为不同的二进制格式，它们的区别在于编码方式和用途不同，嵌入式场景转换为binary比ihex体积大的原因在于binary严格按 物理地址输出，若地址不连续（如 `.text` 在 `0x0000`，`.data` 在 `0x1000`），中间空档会填充 `0x00` 或 `0xFF`。

简单来说：

* `-O binary`生成 原始内存镜像，包含所有段和填充，适合直接烧录，但文件较大。适用场景：Bootloader、Flash 编程、需严格地址对齐的硬件。
* `-O ihex`生成 结构化文本，仅包含有效数据，文件小但需解析。适用场景：需要烧录器支持解析HEX格式、节省存储空间、非连续地址写入。

### 1.3 节区（Section）操作

控制节区的复制、删除、修改等。

| 选项                                | 长选项                                    | 说明                                                         |
| ---------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| -j | --only-section \<name\>            | 仅复制指定节区到输出文件                                     |
| -R | --remove-section \<name\>         | 从输出中删除指定节区                                         |
| -S | --strip-all | 移除目标文件中的所有 符号表（Symbol Table）和 重定位信息（Relocation Information），生成一个更精简的二进制文件 |
| -g | --strip-debug | 移除调试信息 |
|                   | --keep-section \<name\>                  | 不要剥离该节区（配合 `-S` 等使用）                           |
|  | --rename-section \<old\>=\<new\>[,\<flags\>] | 将节区 `<old>` 重命名为 `<new>`，可指定标志                  |
|      | --set-section-flags \<name\>=\<flags\>     | 设置节区 `<name>` 的属性（如 `alloc`, `load`, `readonly` 等） |
|  | --set-section-alignment \<name\>=\<align\> | 设置节区对齐方式（字节对齐）                                 |

**移除调试符号和重定位信息，减少文件体积，防止逆向工程:**

~~~sh
$ riscv64-unknown-elf-objcopy -S -O binary helloworld.elf helloworld.bin
~~~

### 1.4 符号（Symbol）处理

控制符号的保留、删除、重命名、属性修改等。

| 选项 | 长选项                                                    | 说明                                             |
| ---- | --------------------------------------------------------- | ------------------------------------------------ |
| -N   | --strip-symbol \<name\>                                   | 不复制符号 `<name>`                              |
|      | --strip-unneeded-symbol \<name\>                          | 仅当符号被重定位引用时才保留                     |
| -K   | --keep-symbol \<name\>                                    | 保留符号 `<name>`（即使使用 `-S`）               |
| -L   | --localize-symbol \<name\>                                | 强制将符号 `<name>` 标记为局部符号               |
| -G   | --globalize-symbol \<name\>                               | 强制将符号 `<name>` 标记为全局符号               |
|      | --keep-global-symbol \<name\>                             | 仅保留 `<name>` 为全局符号，其余全局符号转为局部 |
| -W   | --weaken-symbol \<name\>                                  | 强制将符号 `<name>` 标记为弱符号                 |
|      | --weaken                                                  | 将所有全局符号标记为弱符号                       |
|      | --redefine-sym \<old\>=\<new\>                            | 将符号 `<old>` 重命名为 `<new>`                  |
|      | --redefine-syms \<file\>                                  | 批量重命名符号（从文件读取 `old=new` 列表）      |
|      | --prefix-symbols \<prefix\>                               | 给所有符号名前添加前缀                           |
|      | --add-symbol \<name\>=[\<section\>:]\<value\>[,\<flags\>] | 添加一个新符号                                   |
| -x   | --discard-all                                             | 删除所有非全局符号                               |
| -X   | --discard-locals                                          | 删除编译器生成的局部符号（如 `.L*`）             |
|      | --keep-file-symbols                                       | 不删除文件符号（通常为源文件名）                 |
|      | --keep-section-symbols                                    | 不删除节区符号                                   |


### 1.5 地址与布局控制

调整程序加载地址、虚拟地址、填充等。

| 选项                                                      | 说明                                        |
| --------------------------------------------------------- | ------------------------------------------- |
| --change-addresses, --adjust-vma \<incr\>                 | 给所有节区的 VMA、LMA 和起始地址加 `<incr>` |
| --change-section-address, --adjust-section-vma \<name>\{= | +                                           |
| --change-section-lma \<name\>{=                           | +                                           |
| `--change-section-vma <name>{=|+|-}<val>`                 | 修改节区 `<name>` 的 VMA（虚拟地址）        |
| --set-start \<addr\>                                      | 设置程序起始地址（入口点）                  |
| --adjust-start \<incr\>                                   | 起始地址增加 `<incr>`                       |
| --pad-to \<addr\>                                         | 将最后一个节区填充到地址 `<addr>`           |
| --gap-fill \<val\>                                        | 用字节值 `<val>` 填充节区间隙               |
| --reverse-bytes=\<num\>                                   | 在输出中每 `<num>` 字节进行字节序反转       |


### 1.6 其他通用选项

| 选项                | 长选项                 | 说明                               |
| --------------------- | ---------------------------------- | ---------------------------------- |
| -v   | --verbose     | 显示处理的文件列表（详细输出）     |
| -V   | --version     | 显示版本信息                       |
| -h      | --help        | 显示帮助                           |
|              |--info              | 列出支持的目标格式和架构           |



## 2 典型使用场景

| 目的                   | 命令示例                                                     |
| ---------------------- | ------------------------------------------------------------ |
| 转换 ELF 为纯二进制    | riscv64-unknown-elf-objcopy -O binary input.elf output.bin   |
| 提取 `.text` 节区      | riscv64-unknown-elf-objcopy -j .text input.elf output.elf    |
| 去除所有符号和调试信息 | riscv64-unknown-elf-objcopy -S input.elf output.elf          |
| 仅保留调试信息         | riscv64-unknown-elf-objcopy --only-keep-debug input.elf debug.info |
| 添加调试链接           | riscv64-unknown-elf-objcopy --add-gnu-debuglink=debug.info output.bin |
| 重命名节区             | riscv64-unknown-elf-objcopy --rename-section .data=.mydata input.elf output.elf |
| 修改起始地址           | riscv64-unknown-elf-objcopy --change-addresses 0x80000000 input.elf output.elf |
| 批量重命名符号         | riscv64-unknown-elf-objcopy --redefine-syms=syms.txt input.elf output.elf |

