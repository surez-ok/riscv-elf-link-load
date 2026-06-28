# 基于RISC-V的编译链接和装载03之ELF文件头详解

[TOC]

ELF文件头（ELF Header）包含了整个文件的基本属性，比如ELF的文件版本，目标机器型号，程序入口地址，文件头长度等。

## 1 使用`readelf -h`查看文件头

上一篇已经讲到使用`readelf -h`来查看文件头，这里抄录如下：

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
| **浮点 ABI**   | double-float ABI   | 使用双精度浮点寄存器传递参数        |



## 2 ELF文件头的数据结构

以rv64为例，其文件头数据结构为（rv32成员也与一样，只是数据类型不一样，所以文件头长度也不一样）：

~~~c
// https://github.com/roolebo/elfutils/blob/master/libelf/elf.h

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf64_Half	e_type;			    /* Object file type */
  Elf64_Half	e_machine;		    /* Architecture */
  Elf64_Word	e_version;		    /* Object file version */
  Elf64_Addr	e_entry;		    /* Entry point virtual address */
  Elf64_Off	    e_phoff;		    /* Program header table file offset */
  Elf64_Off	    e_shoff;		    /* Section header table file offset */
  Elf64_Word	e_flags;		    /* Processor-specific flags */
  Elf64_Half	e_ehsize;		    /* ELF header size in bytes */
  Elf64_Half	e_phentsize;		/* Program header table entry size */
  Elf64_Half	e_phnum;		    /* Program header table entry count */
  Elf64_Half	e_shentsize;		/* Section header table entry size */
  Elf64_Half	e_shnum;		    /* Section header table entry count */
  Elf64_Half	e_shstrndx;		    /* Section header string table index */
} Elf64_Ehdr;
~~~

ELF文件头结构成员含义：

| 序号 | 成员               | 含义                                                         |
| ---- | ------------------ | ------------------------------------------------------------ |
| 1    | e_ident[EI_NIDENT] | ELF 标识，16字节                                             |
| 2    | e_type             | 文件类型                                                     |
| 3    | e_machine          | 目标机器架构                                                 |
| 4    | e_version          | ELF 版本                                                     |
| 5    | e_entry            | 程序入口地址（rv64这一字段为8Byte， rv32 这一字段为4Byte）   |
| 6    | e_phoff            | Program Header 表偏移（rv64这一字段为8Byte， rv32 这一字段为4Byte） |
| 7    | e_shoff            | Section Header 表偏移（rv64这一字段为8Byte， rv32 这一字段为4Byte） |
| 8    | e_flags            | 处理器特定标志                                               |
| 9    | e_ehsize           | ELF Header 大小                                              |
| 10   | e_phentsize        | Program Header 项大小                                        |
| 11   | e_phnum            | Program Header 项数量                                        |
| 12   | e_shentsize        | Section Header 项大小                                        |
| 13   | e_shnum            | Section Header 项数量                                        |
| 14   | e_shstrndx         | 节名字符串表索引                                             |

**注意：** RV32文件头结构体成员也与RV64一样，只是数据类型不一样，所以文件头长度也不一样，

* RV32 位 ELF 文件头共52 字节

* RV64 位 ELF 文件头共64 字节

  RV32与RV64  差异在于上表标出的3个成员数据类型不一样。

## 3 ELF文件头的数据结构各字段详解

### 3.1 ELF 标识： e_ident[EI_NIDENT] 

| 偏移 | 名称          | 含义                                                         |
| ---- | ------------- | ------------------------------------------------------------ |
| 0-3  | e_ident[0-3]  | 魔数（Magic Number）：0x7f, 0x45, 0x4c, 0x46,<br />`0x7F 'E' 'L' 'F'`（固定值，标识这是 ELF 文件） |
| 4    | e_ident[4]    | 类别（Class）：<br>`0`: 无效<br>`1`: 32 位 ELF<br>`2`: 64 位 ELF |
| 5    | e_ident[5]    | 字节序（Data）：<br>`0`: 无效<br>`1`: 小端序（Little Endian）<br>`2`: 大端序（Big Endian） |
| 6    | e_ident[6]    | ELF 版本（Version）：<br>通常是 `1`（EV_CURRENT）            |
| 7    | e_ident[7]    | OS ABI：<br>标识操作系统 ABI，如：<br>`0`: System V<br>`3`: Linux<br>`6`: FreeBSD 等 |
| 8    | e_ident[8]    | ABI 版本：ABI 子版本                                         |
| 9-15 | e_ident[9-15] | 填充字节：保留，通常为 0                                     |

**举例：**

helloword 的前16个字节如下所示：

~~~log
0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
~~~

* e_ident[0-3]：`7F 45 4C 46` ，即`0x7F 'E' 'L' 'F'`，是所有 ELF 文件的标识
* e_ident[4]:  0x02，表示rv64
* e_ident[5]：0x01，表示小端序
* e_ident[6]：0x01，表示ELF 版本，通常为1
* e_ident[7]：0x0，表示OS ABI 为System V
* e_ident[8]：0x0 ，表示ABI子版本
* e_ident[9-15] 保留



### 3.2 文件类型：e_type

| 值   | 宏定义  | 含义                                                         |
| ---- | ------- | ------------------------------------------------------------ |
| 0    | ET_NONE | 未知类型                                                     |
| 1    | ET_REL  | 可重定位文件（`.o` 目标文件）                                |
| 2    | ET_EXEC | 固定地址加载的可执行文件（非 PIE）。可以是静态链接，也可以是非 PIE 的动态链接 |
| 3    | ET_DYN  | 共享对象（`.so` 动态库）或 PIE 可执行文件（支持加载地址随机化）。现代默认的动态链接程序属于此类 |
| 4    | ET_CORE | Core dump 文件（程序崩溃时内存快照）                         |

上一篇文章提到如下4类文件类型都是ELF格式，可以与这里的e_type对应：

**属于ELF格式的4种文件类型：**

| ELF文件类型              | e_type  | 说明                                                         | 正确的实例                           |
| :----------------------- | :------ | :----------------------------------------------------------- | :----------------------------------- |
| 可执行文件（旧式/非PIE） | ET_EXEC | 固定地址加载，不支持ASLR。常见于老旧系统或加了 `-no-pie` 编译的程序。 | 老旧嵌入式固件、特定编译选项下的程序 |
| 可执行文件（现代/PIE     | ET_DYN  | 支持加载地址随机化的可执行文件。现代Linux默认编译方式。      | `/usr/bin/ls`**、**`/usr/bin/bash 等 |
| 目标文件                 | ET_REL  | 需要链接重定位。静态库（`.a`）本质是此类文件的集合。         | 编译产生的 `.o` 文件，链接器使用     |
| 共享库                   | ET_DYN  | 动态链接库。注意：它与 PIE 可执行文件共享同一个 `e_type` 值，系统根据加载方式区分。 | linux下的.so库                       |
| 核心转储                 | ET_CORE | 进程崩溃时的内存快照。                                       | linux下的core dump                   |

**注意：**

并非静态链接ELF的文件类型一定为`ET_EXEC` ， 动态链接ELF的文件类型一定为`ET_DYN`类型 ，`ET_EXE`与`ET_DYN`类型的区别在于是否固定加载地址，这由编译时是否开启 `-pie` 决定。

那么加载器是怎么区分静态链接ELF还是动态链接ELF呢？

装载器是通过ELF文件程序头表（Program Header）中是否有 `PT_INTERP`段（Interpreters，解释器/动态链接器）来判断是静态链接还是动态链接ELF（见第12章描述）

### 3.3 目标机器架构：e_machine
表示该文件运行在哪种 CPU 架构上，相关常量常以EM_开头：

| 值（十六进制） | 值（十进制） | 架构       | 示例            |
| -------------- | ------------ | ---------- | --------------- |
| 0x2            | 2            | EM_SPARC   | SPARC           |
| 0x3            | 3            | EM_386     | Intel 80386     |
| 0x8            | 8            | EM_MIPS    | MIPS            |
| 0x28           | 40           | EM_ARM     | ARM             |
| 0x3E           | 62           | EM_X86_64  | x86-64（AMD64） |
| 0xB7           | 183          | EM_AARCH64 | ARM64           |
| 0xF3           | 243          | EM_RISCV   | RISC-V          |

### 3.4 ELF 版本：e_version

当前必须为 1（EV_CURRENT），用于向后兼容

### 3.5 程序入口地址：e_entry

程序开始执行的虚拟地址，加载器会跳转到这个地址开始执行：

- 对于可执行文件（ET_EXEC），这是 `_start` 或 `main` 的地址
- 对于可重定位文件（`.o`文件），此值为 0（可重定位文件无入口地址）

### 3.6 Program Header 表偏移： e_phoff 

Program Header Table 在文件中的字节偏移量，用于加载器读取段信息（如代码段、数据段）

**注意：**如果没有程序头（如 `.o` 文件），则为 0

### 3.7 Section Header 表偏移：e_shoff 

Section Header Table 在文件中的字节偏移量，用于链接、调试、查看符号等

**注意：**如果没有节头（如 stripped 的二进制），则为 0

### 3.8 处理器特定标志：e_flags

e_flags与机器架构（ e_machine字段）相关，不同架构有不同的标志位，例如，MIPS: 指定是否使用浮点协处理器，ARM: 指定 Thumb 模式等，RISC-V 的 e_flags使用以下位域（bitmask），细节见参考1：

| 标志（Flag）              | 值（十六进制） | 含义                                                         |
| ------------------------- | -------------- | ------------------------------------------------------------ |
| EF_RISCV_FLOAT_ABI_SOFT   | 0x0000         | 软浮点（soft）                                               |
| EF_RISCV_RVC              | 0x0001         | 使用RVC（RISC-V Compressed）指令扩展（`.text` 中包含 `C` 指令） |
| EF_RISCV_FLOAT_ABI_SINGLE | 0x0002         | 单精度浮点（single）                                         |
| EF_RISCV_FLOAT_ABI_DOUBLE | 0x0004         | 双精度浮点（double）                                         |
| EF_RISCV_FLOAT_ABI        | 0x0006         | 浮点 ABI 类型（2 位）                                        |
| EF_RISCV_FLOAT_ABI_QUAD   | 0x0006         | 四倍精度（罕见）                                             |
| EF_RISCV_RVE              | 0x0008         | 使用 Embedded RISC-V（RVE）整数扩展（精简寄存器版本）        |
| EF_RISCV_TSO              | 0x0010         | 使用 Total Store Order (TSO) 内存模型（实验性）              |

**RISC-V常见链接报错问题示例：**

编译链接时，常碰到如下链接错误，见参考2描述：

~~~sh
can't link soft-float modules with double-float modules 
~~~

这个链接错误 `can't link soft-float modules with double-float modules` 是 RISC-V 开发中非常典型的 **ABI 不匹配** 问题。

在项目中，部分目标文件（`.o`）或静态库（`.a`）在编译时使用的是软浮点（soft-float），而另一部分使用的是双精度硬浮点（double-float）时，链接器就会拒绝将它们合并。所以我们需要找出哪些目标文件是soft-float。

出现这种问题的可能原因：

1. 编译时是否传入正确的march与mabi
2. 项目是否没有clean干净，导致有残留的.o文件？ 可以在clean后，查看项目中是否有.o文件

排查方法，使用如下命令：

~~~sh
$ find . -name "*.o" | xargs riscv64-unknown-linux-gnu-readelf -h 2>/dev/null | grep -E "File:|soft-float"
~~~

这个命令是一个组合管道命令，主要用于批量检查当前目录下所有 `.o`（目标文件）的 ELF 头部信息，并从中筛选出文件名以及是否包含“soft-float”（软浮点）的标识。

### 3.9 ELF Header 大小：e_ehsize

ELF Header 自身的大小（字节）：

* 32 位系统：52 字节

- 64 位系统：64 字节

### 3.10 程序头（Program Header） 项大小：e_phentsize 

每个Program Header Entry 的大小（字节）：

* 32 位系统：32 字节

* 64 位系统：56 字节

**注意：**如果没有程序头（如 `.o` 文件），则为 0

### 3.11 程序头（Program Header） 项数量：e_phnum

Program Header Table 中有多少个条目，结合 `e_phoff` 和 `e_phentsize`，可以定位所有段

**注意：**如果 `e_phnum` 是 `0`，表示没有 Program Header Table

### 3.12 节头（Section Header） 项大小：e_shentsize 

每个Section Header Entry的大小：

* 32 位：40 字节

- 64 位：64 字节

### 3.13 节头（Section Header） 项数量： e_shnum 

Section Header Table 中有多少个节（section），如 `.text`, `.data`, `.symtab`, `.strtab` 等

### 3.14 节名字符串表索引：e_shstrndx 

存储节名称的字符串表（`.shstrtab`）在 Section Header Table 中的索引，例如：值为 34，表示第 34 个 section 是 `.shstrtab`，通过它才能知道每个 section 的名字（如 `.text`）



**参考：**

1. https://riscv-non-isa.github.io/riscv-elf-psabi-doc/
1. [RISC-V Binutils工具链03之readelf工具的使用]