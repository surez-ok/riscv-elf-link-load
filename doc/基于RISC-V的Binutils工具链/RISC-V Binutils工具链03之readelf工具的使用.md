# RISC-V Binutils工具链03之readelf工具的使用

[TOC]

readelf 用于查看ELF（Executable and Linkable Format）格式的二进制文件。

## 1 readelf  工具常用选项

readelf 工具常用选项如下：

~~~sh
$ riscv64-unknown-linux-gnu-readelf --help
~~~

输出log见：[riscv64-unknown-linux-gnu-readelf_help](./images/riscv64-unknown-linux-gnu-readelf_help.log)

| 选项 | 长选项                          | 含义                                                   |
| ---- | ------------------------------- | ------------------------------------------------------ |
| -a   | --all                           | 输出常用信息，等价于-h -l -S -s -r -d -V -A -I         |
| -h   | --file--header                  | 输出ELF文件头                                          |
| -l   | --program-headers               | 输出程序头（段头）                                     |
|      | --segments                      | 与-l以及--program-headers等价                          |
| -S   | --section-headers               | 输出节头                                               |
|      | --sections                      | 于-S以及--section-headers等价                          |
| -g   | --section-groups                | 输出节组信息                                           |
| -t   | --section-details               | 输出比-S更详细的节信息                                 |
| -e   | --headers                       | 输出3种头信息，等价于-h -l -S                          |
| -s   | --syms                          | 输出符号表信息                                         |
|      | --symbols                       | 与--syms等价                                           |
|      | --dyn-syms                      | 输出动态符号表                                         |
|      | --lto-syms                      | 输出LTO符号表                                          |
| -n   | --notes                         | 输出NOTE段和.note节的内容(如果存在)                    |
| -r   | --relocs                        | 输出.rel节即重分配节的内容(如果存在)，以及可重定位类型 |
| -u   | --unwind                        | 输出.unwind节的内容(如果存在)                          |
| -d   | --dynamic                       | 输出.dyn节的内容                                       |
| -V   | --version-info                  | 输出符号的版本信息                                     |
| -A   | --arch-specific                 | 输出体系架构信息                                       |
| -D   | --use-dynamic                   | 输出符号信息时，不用.sym节，而用.dyn节信息来表示       |
| -p   | --string-dump=\<number\|name>   | 将节的内容按照字符串展示                               |
| -R   | --relocated-dump=<number\|name> | 用于输出指定段（section）经过重定位后的内容            |
| -x   | --hex-dump=<number\|name>       | 把索引为N的节一16进制输出                              |
| -w   | -debug-dump                     | 输出.debug节的内容                                     |
| -I   | --histogram                     | 表示符号表内容时，显示斗列表的长度的直方图             |
| -W   | --wide                          | 输出时使用宽屏模式，尽量不换行                         |
| -v   | --version                       | 输出readelf工具的版本信息                              |
| -H   | --help                          | 输出readelf命令的帮助信息                              |



## 2 readelf  常用选项举例

以下对一些命令进行举例：

### 2.1 查看ELF文件头

~~~sh
$ riscv64-unknown-linux-gnu-readelf -h helloworld.elf
~~~

其输出为：

~~~log
$ riscv64-unknown-linux-gnu-readelf -h helloworld.elf
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           RISC-V
  Version:                           0x1
  Entry point address:               0x10468
  Start of program headers:          64 (bytes into file)
  Start of section headers:          8128 (bytes into file)
  Flags:                             0x5, RVC, double-float ABI
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         10
  Size of section headers:           64 (bytes)
  Number of section headers:         35
  Section header string table index: 34
~~~

可以看到如下信息：

| 属性           | 值                 | 意义                                |
| -------------- | ------------------ | ----------------------------------- |
| **架构**       | RISC-V 64-bit      | 目标平台为 64 位 RISC-V             |
| **字节序**     | Little Endian      | 小端模式                            |
| **文件类型**   | 可执行文件（EXEC） | 可直接运行，无需链接                |
| **入口地址**   | `0x10468`          | 程序从这个地址开始执行              |
| **程序头数量** | 10                 | 有 10 个段（Segment）需要加载到内存 |
| **节的数量**   | 35                 | 包含大量调试/链接信息               |
| **使用扩展**   | RVC（压缩指令）    | 代码更紧凑，节省空间                |
| **浮点 ABI**   | `lp64d`            | 使用双精度浮点寄存器传递参数        |

**常见ABI 不匹配问题：**

编译链接时，常碰到如下链接错误：

~~~sh
can't link soft-float modules with double-float modules 
~~~

这个链接错误 `can't link soft-float modules with double-float modules` 是 RISC-V 开发中非常典型的 **ABI 不匹配** 问题。

在项目中，部分目标文件（`.o`）或静态库（`.a`）在编译时使用的是软浮点（soft-float），而另一部分使用的是双精度硬浮点（double-float）时，链接器就会拒绝将它们合并。所以我们需要找出哪些目标文件是soft-float。

出现这种问题的可能原因：

1. 编译链接时是否传入正确的march与mabi
2. 项目是否没有clean干净，导致有残留的.o文件？ 可以在clean后，查看项目中是否有.o文件

排查方法，使用如下命令：

~~~sh
$ find . -name "*.o" | xargs riscv64-unknown-linux-gnu-readelf -h 2>/dev/null | grep -E "File:|soft-float"
~~~

这个命令是一个组合管道命令，主要用于批量检查当前目录下所有 `.o`（目标文件）的 ELF 头部信息，并从中筛选出文件名以及是否包含“soft-float”（软浮点）的标识。

### 2.2 查看节(section)

使用如下命令查看节头（Section header table）：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -S helloworld.elf
~~~

**注意：**使用-SW宽屏显示更方便查看

其输出为：

~~~log
$ riscv64-unknown-linux-gnu-readelf -SW helloworld.elf
There are 35 section headers, starting at offset 0x1e58:

Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .interp           PROGBITS        0000000000010270 000270 000021 00   A  0   0  1
  [ 2] .hash             HASH            0000000000010298 000298 000024 04   A  4   0  8
  [ 3] .gnu.hash         GNU_HASH        00000000000102c0 0002c0 000030 00   A  4   0  8
  [ 4] .dynsym           DYNSYM          00000000000102f0 0002f0 000060 18   A  5   1  8
  [ 5] .dynstr           STRTAB          0000000000010350 000350 00004a 00   A  0   0  1
  [ 6] .gnu.version      VERSYM          000000000001039a 00039a 000008 02   A  4   0  2
  [ 7] .gnu.version_r    VERNEED         00000000000103a8 0003a8 000030 00   A  5   1  8
  [ 8] .rela.plt         RELA            00000000000103d8 0003d8 000030 18  AI  4  20  8
  [ 9] .plt              PROGBITS        0000000000010410 000410 000040 10  AX  0   0 16
  [10] .text             PROGBITS        0000000000010450 000450 0000ba 00  AX  0   0  4
  [11] .rodata           PROGBITS        0000000000010510 000510 000015 00   A  0   0  8
  [12] .eh_frame_hdr     PROGBITS        0000000000010528 000528 000024 00   A  0   0  4
  [13] .eh_frame         PROGBITS        0000000000010550 000550 000060 00   A  0   0  8
  [14] .note.ABI-tag     NOTE            00000000000105b0 0005b0 000020 00   A  0   0  4
  [15] .preinit_array    PREINIT_ARRAY   0000000000011df0 000df0 000008 08  WA  0   0  1
  [16] .init_array       INIT_ARRAY      0000000000011df8 000df8 000008 08  WA  0   0  8
  [17] .fini_array       FINI_ARRAY      0000000000011e00 000e00 000008 08  WA  0   0  8
  [18] .dynamic          DYNAMIC         0000000000011e08 000e08 0001e0 10  WA  5   0  8
  [19] .got              PROGBITS        0000000000011fe8 000fe8 000008 08  WA  0   0  8
  [20] .got.plt          PROGBITS        0000000000011ff0 000ff0 000020 08  WA  0   0  8
  [21] .sdata            PROGBITS        0000000000012010 001010 000008 00  WA  0   0  8
  [22] .bss              NOBITS          0000000000012018 001018 000008 00  WA  0   0  1
  [23] .comment          PROGBITS        0000000000000000 001018 00003d 01  MS  0   0  1
  [24] .riscv.attributes RISCV_ATTRIBUTES 0000000000000000 001055 00005f 00      0   0  1
  [25] .debug_aranges    PROGBITS        0000000000000000 0010b4 000030 00      0   0  1
  [26] .debug_info       PROGBITS        0000000000000000 0010e4 0000b3 00      0   0  1
  [27] .debug_abbrev     PROGBITS        0000000000000000 001197 000068 00      0   0  1
  [28] .debug_line       PROGBITS        0000000000000000 0011ff 000088 00      0   0  1
  [29] .debug_str        PROGBITS        0000000000000000 001287 0000e2 01  MS  0   0  1
  [30] .debug_line_str   PROGBITS        0000000000000000 001369 000056 01  MS  0   0  1
  [31] .debug_rnglists   PROGBITS        0000000000000000 0013bf 000017 00      0   0  1
  [32] .symtab           SYMTAB          0000000000000000 0013d8 0006d8 18     33  56  8
  [33] .strtab           STRTAB          0000000000000000 001ab0 000247 00      0   0  1
  [34] .shstrtab         STRTAB          0000000000000000 001cf7 00015c 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), p (processor specific)
~~~

### 2.3  查看程序头(segment)

使用命令查看程序头信息（Program header table），也即查看段（segment）：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -l helloworld.elf
~~~

其输出为：

~~~log
$ riscv64-unknown-linux-gnu-readelf -lW helloworld.elf

Elf file type is EXEC (Executable file)
Entry point 0x10468
There are 10 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000010040 0x0000000000010040 0x000230 0x000230 R   0x8
  INTERP         0x000270 0x0000000000010270 0x0000000000010270 0x000021 0x000021 R   0x1
      [Requesting program interpreter: /lib/ld-linux-riscv64-lp64d.so.1]
  RISCV_ATTRIBUT 0x001055 0x0000000000000000 0x0000000000000000 0x00005f 0x000000 R   0x1
  LOAD           0x000000 0x0000000000010000 0x0000000000010000 0x0005d0 0x0005d0 R E 0x1000
  LOAD           0x000df0 0x0000000000011df0 0x0000000000011df0 0x000228 0x000230 RW  0x1000
  DYNAMIC        0x000e08 0x0000000000011e08 0x0000000000011e08 0x0001e0 0x0001e0 RW  0x8
  NOTE           0x0005b0 0x00000000000105b0 0x00000000000105b0 0x000020 0x000020 R   0x4
  GNU_EH_FRAME   0x000528 0x0000000000010528 0x0000000000010528 0x000024 0x000024 R   0x4
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x000df0 0x0000000000011df0 0x0000000000011df0 0x000210 0x000210 R   0x1

 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .riscv.attributes
   03     .interp .hash .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.plt .plt .text .rodata .eh_frame_hdr .eh_frame .note.ABI-tag
   04     .preinit_array .init_array .fini_array .dynamic .got .got.plt .sdata .bss
   05     .dynamic
   06     .note.ABI-tag
   07     .eh_frame_hdr
   08
   09     .preinit_array .init_array .fini_array .dynamic .got
~~~

这些信息可以用来查看加载地址LMA->对应着上述PhysAddr，虚拟地址VA->对应着上述VirtAddr

### 2.4 查看编译时用的march

RISC-V支持许多种扩展，有时候我们需要知道某个可执行文件编译时用了什么扩展，可以用如下命令：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -A helloworld.elf
Attribute Section: riscv
File Attributes
  Tag_RISCV_stack_align: 16-bytes
  Tag_RISCV_arch: "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_v1p0_zicsr2p0_zifencei2p0_zmmul1p0_zaamo1p0_zalrsc1p0_zca1p0_zcd1p0_zve32f1p0_zve32x1p0_zve64d1p0_zve64f1p0_zve64x1p0_zvl128b1p0_zvl32b1p0_zvl64b1p0"
~~~

或者使用如下命令：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -p .riscv.attributes helloworld.elf

String dump of section '.riscv.attributes':
  [     0]  A
  [     5]  riscv
  [    13]  rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_v1p0_zicsr2p0_zifencei2p0_zmmul1p0_zaamo1p0_zalrsc1p0_zca1p0_zcd1p0_zve32f1p0_zve32x1p0_zve64d1p0_zve64f1p0_zve64x1p0_zvl128b1p0_zvl32b1p0_zvl64b1p0
~~~



其中，Tag_RISCV_arch表示所依赖的RISC-V 指令集架构（ISA）及其各个扩展的版本号

含义如下：

| 字符串    | ISA                                        | 版本 |
| --------- | ------------------------------------------ | ---- |
| rv64i2p0  | `I` 扩展（基础整数指令集）                 | v2.0 |
| m2p0      | `M` 扩展（整数乘法和除法指令）             | v2.0 |
| f2p0      | `F` 扩展（单精度浮点指令）                 | v2.0 |
| d2p0      | `D` 扩展（双精度浮点指令）                 | v2.0 |
| c2p0      | `C` 扩展（16位压缩指令）                   | v2.0 |
| zmmul1p0  | 仅包含整数乘法（不包含除法）的扩展         | v1.0 |
| zaamo1p0  | 原子内存操作（AMO）扩展                    | v1.0 |
| zalrsc1p0 | Load-Reserved / Store-Conditional 指令扩展 | v1.0 |
| zca1p0    | 压缩指令的基础扩展                         | v1.0 |
| zcd1p0    | 压缩的双精度浮点指令扩展                   | v1.0 |

### 2.5 查看编译时用的参数

有时候我们需要知道某个可执行文件编译时用了什么参数，可以用如下命令（需要编译可执行文件时候打开-g）：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -p .debug_str helloworld.elf

String dump of section '.debug_str':
  [     0]  long unsigned int
  [    12]  GNU C23 15.1.0 -mabi=lp64d -misa-spec=20191213 -mtls-dialect=trad -march=rv64imafdc_zicsr_zmmul_zaamo_zalrsc_zca_zcd -g -O2
  [    8e]  short unsigned int
  [    a1]  short int
  [    ab]  __builtin_puts
  [    ba]  unsigned char
  [    c8]  long int
  [    d1]  main
~~~

可见编译参数为：`-march=rv64imafdc_zmmul_zaamo_zalrsc_zca_zcd -g -O2`

或者用strings命令(第12篇会专门讲)：

~~~sh
$ riscv64-unknown-elf-strings helloworld.elf | grep GNU  

GCC: (GNU) 15.1.0
GNU C23 15.1.0 -mabi=lp64d -misa-spec=20191213 -mtls-dialect=trad -march=rv64imafdc_zicsr_zmmul_zaamo_zalrsc_zca_zcd -g -O2
__GNU_EH_FRAME_HDR
~~~



**参考：**

1. https://github.com/roolebo/elfutils/blob/master/src/readelf.c