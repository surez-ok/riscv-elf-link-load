# RISC-V Binutils工具链10之size工具的使用

[TOC]

size工具主要用于报告目标文件或库文件的各个段（segment）的大小。这样帮助开发者了解编译后的二进制文件中代码段、数据段等各部分的大小。

## 1 size工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-size --help
~~~

输出log见：[riscv64-unknown-linux-gnu-size_help](images/riscv64-unknown-linux-gnu-size_help.log)

| 选项 | 长选项            | 含义                                 |
| ---- | ----------------- | ------------------------------------ |
| -A   | --format=sysv     | 使用 System V 风格输出（表格形式）   |
| -B   | --format=berkeley | 使用 Berkeley 风格输出（简洁，默认） |
| -d   | -radix=10         | 以 10进制显示大小（默认）            |
| -x   | --radix=16        | 以 16进制显示大小                    |
| -t   | --totals          | 显示所有文件的总大小                 |
| -h   | --help            | 显示帮助信息                         |



## 2 典型使用场景

### 2.1 size 工具的基本使用

size 用来测量目标文件或库文件的各个段（segment）的大小：

~~~sh
$ riscv64-unknown-linux-gnu-size helloworld.elf
~~~

输出为：

~~~log
$ riscv64-unknown-linux-gnu-size helloworld.elf
   text    data     bss     dec     hex filename
    810     552       8    1370     55a helloworld.elf
~~~

表示：helloworld.elf的text段为810字节，data段为552字节，bss段为8字节，总大小为810 + 552 + 8 = 1370 字节 （十六进制0x55a字节）

### 2.2 输出节的大小

size工具默认输出各个段（segment）的大小，但是我们也可以用如下指令输出各个节（section）的大小：

~~~sh
$ riscv64-unknown-linux-gnu-size -A helloworld.elf
~~~

输出为：

~~~log
$ riscv64-unknown-linux-gnu-size -A helloworld.elf
helloworld.elf  :
section             size    addr
.interp               33   66160
.hash                 36   66200
.gnu.hash             48   66240
.dynsym               96   66288
.dynstr               74   66384
.gnu.version           8   66458
.gnu.version_r        48   66472
.rela.plt             48   66520
.plt                  64   66576
.text                170   66640
.rodata               21   66816
.eh_frame_hdr         36   66840
.eh_frame             96   66880
.note.ABI-tag         32   66976
.preinit_array         8   73200
.init_array            8   73208
.fini_array            8   73216
.dynamic             480   73224
.got                   8   73704
.got.plt              32   73712
.sdata                 8   73744
.bss                   8   73752
.comment              45       0
.riscv.attributes    202       0
.debug_aranges        48       0
.debug_info          184       0
.debug_abbrev        112       0
.debug_line          132       0
.debug_str           214       0
.debug_line_str       67       0
.debug_rnglists       23       0
Total               2397
~~~

我们一般关心段（segment）的大小，比如代码段多大？数据段多大等，那么节（section）大小怎么与段（segment）对照呢？可以这么做：

先获取段和节的映射关系，然后将节的大小加起来，然后与段（segment）对照。

**获取段和节的映射关系：**

~~~sh
$ riscv64-unknown-linux-gnu-readelf -l helloworld.elf
~~~

输出log为

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
  RISCV_ATTRIBUT 0x001045 0x0000000000000000 0x0000000000000000 0x0000ca 0x000000 R   0x1
  LOAD           0x000000 0x0000000000010000 0x0000000000010000 0x0005c0 0x0005c0 R E 0x1000
  LOAD           0x000df0 0x0000000000011df0 0x0000000000011df0 0x000228 0x000230 RW  0x1000
  DYNAMIC        0x000e08 0x0000000000011e08 0x0000000000011e08 0x0001e0 0x0001e0 RW  0x8
  NOTE           0x0005a0 0x00000000000105a0 0x00000000000105a0 0x000020 0x000020 R   0x4
  GNU_EH_FRAME   0x000518 0x0000000000010518 0x0000000000010518 0x000024 0x000024 R   0x4
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

所以根据节（section）与段（segment）的映射关系：

| 段（segment） | 通过size命令查看的大小      | 节（section）                                                | 按节加起来的结果 |
| ------------- | --------------------------- | ------------------------------------------------------------ | ---------------- |
| text段        | 810 B                       | .interp .hash .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r <br />.rela.plt .plt .text .rodata .eh_frame_hdr.eh_frame .note.ABI-tag | 810 B            |
| data段        | 560 B（552 + 8（.bss 段）） | .preinit_array .init_array .fini_array .dynamic .got .got.plt .bss | 560 B            |

可见：通过size命令查看的大小与按节加起来的结果能够对得上。

### 2.2 对比多个不同ELF的code size

有时候，我们需要比较优化前后的ELF大小是否有变化，可以使用size命令测试多个elf

~~~sh
$ riscv64-unknown-linux-gnu-size helloworld.elf helloworld_static.elf
~~~

有如下log输出，这样我们就可以轻松的对比不同ELF的codesize：

~~~log
$ riscv64-unknown-linux-gnu-size helloworld.elf helloworld_static.elf
text    data     bss     dec     hex filename
    810     552       8    1370     55a helloworld.elf
 424233   21900   21640  467773   7233d helloworld_static.elf
~~~



**参考：**

1. [riscv64-unknown-elf-size(1) — binutils-riscv64-unknown-elf — Debian unstable — Debian Manpages](https://manpages.debian.org/unstable/binutils-riscv64-unknown-elf/riscv64-unknown-elf-size.1.en.html)

