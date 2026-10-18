# 基于RISC-V的编译链接和装载05之ELF之段(segment)

[TOC]

段是加载器处理ELF文件的单位，段由1个或1个以上的节组成，内存上有只读，可读可写，可执行等不同属性，所以可以根据属性加载不同的段。

## 1 使用readelf -l查看程序头

使用`readelf -l`命令查看程序头表（Program header table）：

~~~sh
$ riscv64-unknown-linux-gnu-readelf -l helloworld.elf
~~~

**注意：**使用`-lW`宽屏显示更方便查看

其输出为：

~~~sh
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

从日志中可以得到如下有用信息：

### 1.1 节和段的对应关系

日志中最后部分的 **Section to Segment mapping** 揭示了哪些节被合并到了同一个运行时段中，表明了节与段的映射关系：

比如：

00 段表示Type为PHDR的段，

01段表示Type为INTERP段，

02表示Type为RISCV_ATTRIBUT段，

03表示Type为LOAD段，

04表示Type为LOAD段，与日志上半部分一一对应

程序执行时我们一般只用关注Type为LOAD的段（动态链接还需关注GNU_RELRO段），这几个段与节的对应关系为：

| Segment             | 包含的节                                                     | 运行时权限     | 说明                                                         |
| :------------------ | :----------------------------------------------------------- | :------------- | :----------------------------------------------------------- |
| **03 (LOAD代码段)** | .interp .hash .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.plt .plt .text .rodata .eh_frame_hdr .eh_frame .note.ABI-tag | R E            | 所有只读、可执行的内容被打包在一起，加载后受 MMU 页保护，不可篡改 |
| **04 (LOAD数据段)** | .preinit_array .init_array .fini_array .dynamic .got .got.plt .sdata .bss | RW             | 所有可读写的全局数据和动态链接控制表在此，支持运行时修改     |
| **09 (GNU_RELRO)**  | `.preinit_array`, `.init_array`, `.fini_array`, `.dynamic`, `.got` | 加载后变为只读 | 用于保护初始化函数表和 GOT 表，防止安全攻击                  |

程序头表本身并不直接记录节和段映射，它只包含段的地址、偏移、大小和权限。

`Section to Segment mapping` 输出的映射关系是经过`readelf`解析得到的，解析原理是： `readelf`通过将程序头中每个段的文件偏移范围（`p_offset` 到 `p_offset+p_filesz`）与节头表中每个节的文件偏移范围（`sh_offset` 到 `sh_offset+sh_size`）进行比对，来推断/计算这种映射。



### 1.2  运行时内存段布局

这是加载器实际映射到 RAM 的两个物理段（`LOAD` 类型），决定了实际内存占用：

| 段类型            | 文件偏移   | 虚拟地址 (VirtAddr) | 文件大小 (FileSiz) | 内存大小 (MemSiz) | 权限          | 实际意义                                                     |
| :---------------- | :--------- | :------------------ | :----------------- | :---------------- | :------------ | :----------------------------------------------------------- |
| **LOAD (代码段)** | `0x000000` | `0x10000`           | `0x5c0` (1472B)    | `0x5c0` (1472B)   | R E (读+执行) | 包含代码、只读常量、动态链接表。占用 Flash/RAM 约 1.44 KB（按 4KB 页对齐后实际占 4KB）。 |
| **LOAD (数据段)** | `0x000df0` | `0x11df0`           | `0x228` (552B)     | `0x230` (560B)    | RW(读+写)     | 包含全局变量、GOT 表。运行时实际占用 560 字节（按 4KB 页对齐后实际占 4KB）。 |

### 1.3 BSS 段大小（FileSiz vs MemSiz）
数据段的 `FileSiz`（0x228）比 `MemSiz`（0x230）少了 8 个字节（0x230 - 0x228 = 0x8）。这 8 字节正是之前 `-SW` 中看到的 `.bss` 段大小。说明BSS段不会占用磁盘大小，但是加载后会占用实际的Memory。

ELF 文件中不存储未初始化的全局变量（BSS），但加载器会在内存中 `0x11df0 + 0x228` 的位置额外开辟 8 字节并清零。这证明`.bss` 变量（如未初始化的全局数组）会被正确初始化为 0。

### 1. 4 动态链接器依赖
`INTERP` 段明确指定了请求的程序解释器：`/lib/ld-linux-riscv64-lp64d.so.1`。这是 RISC-V 64 位 glibc 动态链接器的绝对路径。 所以此程序必须在带有标准 glibc 的 RISC-V Linux 系统上运行。

## 2 ELF程序头的数据结构

### 2.1 ELF程序头结构成员含义

以rv64为例，其文件头数据结构为（rv32成员也与一样，只是数据类型不一样，所以文件头长度也不一样）：

~~~c
// https://github.com/roolebo/elfutils/blob/master/libelf/elf.h

typedef struct
{
  Elf64_Word	p_type;			/* Segment type */
  Elf64_Word	p_flags;		/* Segment flags */
  Elf64_Off	    p_offset;		/* Segment file offset */
  Elf64_Addr	p_vaddr;		/* Segment virtual address */
  Elf64_Addr	p_paddr;		/* Segment physical address */
  Elf64_Xword	p_filesz;		/* Segment size in file */
  Elf64_Xword	p_memsz;		/* Segment size in memory */
  Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;
~~~

ELF文件头结构成员含义：

| 序号 | 成员     | 含义                                                         |
| ---- | -------- | ------------------------------------------------------------ |
| 1    | p_type   | 段的类型，决定该段的用途。常见值：<br/>- `PT_NULL` = 0：无效段<br/>- `PT_LOAD` = 1：可加载段（如代码、数据）<br/>- `PT_DYNAMIC` = 2：动态链接信息<br/>- `PT_INTERP` = 3：指定动态链接器路径（如 `/lib/ld-linux.so`）<br/>- `PT_NOTE` = 4：附加信息（如构建 ID）<br/>- `PT_SHLIB` = 5：保留（未使用）<br/>- `PT_PHDR` = 6：程序头表自身的位置<br/>- `PT_TLS` = 7：线程局部存储模板 |
| 2    | p_flags  | 段的权限标志，表示该段在内存中的访问权限：<br/>- `PF_R` = 0x4：可读<br/>- `PF_W` = 0x2：可写<br/>- `PF_X` = 0x1：可执行<br/>例如：`PF_R + PF_X` 表示代码段（`.text`），`PF_R + PF_W` 表示数据段（`.data`）。 |
| 3    | p_offset | 段内容在 ELF 文件中的偏移量（字节为单位）<br/>加载器从该偏移开始读取 `p_filesz` 字节的数据到内存 |
| 4    | p_vaddr  | 段在内存中的虚拟地址（运行时地址）<br/>加载器将该段数据复制到此地址。例如：`0x400000` |
| 5    | p_paddr  | 段在内存中的物理地址<br/>在大多数现代操作系统中（使用虚拟内存），此字段被忽略，仅在无 MMU 的嵌入式系统中可能有用 |
| 6    | p_filesz | 段在文件中的大小（字节）<br/>表示从文件中读取多少数据        |
| 7    | p_memsz  | 段在内存中的大小（字节）<br/>如果 `p_memsz > p_filesz`，多出的部分用零填充（常用于 `.bss` 段<br/>例如：`.bss` 段 `p_filesz = 0`，但 `p_memsz = 1024`，表示分配 1KB 内存并清零 |
| 8    | p_align  | 段的对齐要求（必须是 2 的幂）<br/>`p_vaddr` 和 `p_offset` 都必须对齐到该值<br/>例如：`p_align = 4096` 表示页对齐 |

### 2.2 常见段类型举例

**PT_LOAD：**

**注意：**一个可执行文件至少有一个 PT_LOAD 类型的段。

这类程序头描述的是可装载的段，也就是说，这种类型的段将被装载或者映射到内存中。

例如：一个需要动态链接的 ELF 可执行文件通常包含以下两个可装载的段（类型为 PT_LOAD）：

* 存放程序代码的 text 段，权限为可读/ 可执行；

* 存放全局变量和动态链接信息的 data 段，权限为可读可写。

**PT_DYNAMIC：**

动态段是动态链接可执行文件所特有的，包含了动态链接器所必需的一些信息。在动态段中包含以下内容：

* 运行时需要链接的共享库列表；

* 全局偏移表（.GOT.PLT）的地址，用于ELF 动态链接;

* 重定位条目的相关信息。

**PT_INTERP：**

PT_INTERP 段是将程序解释器路径（如`/lib/ld-linux-riscv64-lp64d.so.1`）存放在一个以 null 为终止符的字符串中。

**PT_NOTE：**

PT_NOTE 类型的段可能保存了与特定供应商或者系统相关的附加信息。

**PT_PHDR：**

PT_PHDR 段保存了程序头表本身的位置和大小。



**参考：**

1. 《程序员的自我修养—链接、装载与库》
