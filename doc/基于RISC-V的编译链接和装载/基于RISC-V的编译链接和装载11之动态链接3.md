# 基于RISC-V的编译链接和装载11之动态链接3

前面几章讲了动态链接过程，但感觉太零碎了，不易理解。这一章用一个实际例子把这些知识点串起来。

[TOC]

## 1 一个动态链接的例子

### 1.1 源码及编译

还是以 `test/04_dyn_got_plt` 为例：编译目标：`add.c`、`sub.c` 编译成共享库（`.so`），`main.c` 与它们动态链接。

**源码：**

```c
// add.c —— 共享库，导出全局变量 g_sum 和函数 add()
int g_sum = 0;             // 全局变量

int add(int a, int b)
{
    g_sum = a + b;         // 经 GOT 间接访问
    return g_sum;
}
```

```c
// sub.c —— 共享库，导出全局变量 g_sub 和函数 sub()
int g_sub = 0;

int sub(int a, int b)
{
    g_sub = a - b;
    return g_sub;
}
```

```c
// main.c —— 主程序，动态链接 add.so / sub.so
#include <stdio.h>

extern int add(int, int);
extern int sub(int, int);

extern int g_sum, g_sub;
int main(void)
{
    add(1, 2);
    add(3, 4);
    sub(5, 6);
    sub(7, 8);
    printf("g_sum = %d, g_sub = %d\r\n", g_sum, g_sub);   // 跨模块读全局变量

    g_sum = 100;                                          // 跨模块写全局变量
    g_sub = 200;                                          // 跨模块写全局变量
    printf("g_sum = %d, g_sub = %d\r\n", g_sum, g_sub);   // 再读验证写入结果

    return 0;
}
```

用例构成如下：其中`add()`/`sub()` 各被调用两次，是为了在观察"第一次调用走动态链接器、第二次调用直接跳转"的延迟绑定现象。

| 文件     | 内容                                                         |
| -------- | ------------------------------------------------------------ |
| `add.c`  | 定义全局变量 `g_sum` 和函数 `add()`                          |
| `sub.c`  | 定义全局变量 `g_sub` 和函数 `sub()`                          |
| `main.c` | 调用 `add()`/`sub()`，读、写共享库里的全局变量 `g_sum`/`g_sub` |

**编译命令（见 Makefile）：**

```sh
# 共享库：位置无关代码
add.so:  riscv64-unknown-linux-gnu-gcc -fPIC -shared add.c -o add.so
sub.so:  riscv64-unknown-linux-gnu-gcc -fPIC -shared sub.c -o sub.so

# 主程序：PIE 可执行文件，动态链接 add.so / sub.so
main:    riscv64-unknown-linux-gnu-gcc -fPIE -pie main.c -o dyn_got_plt.elf add.so sub.so -Wl,-rpath,'$ORIGIN'
```

关键点：

- **`-fPIC`**：共享库必须编译成位置无关代码，因为库加载到哪个地址在运行前是未知的。
- **`-fPIE -pie`**：主程序也编译成位置无关可执行文件，这样主程序访问共享库的全局变量同样通过 GOT表（见 2.2节）。
- **`-Wl,-rpath,'$ORIGIN'`**：运行期动态链接器在可执行文件所在目录查找 `add.so`/`sub.so`。

**运行结果：**

```sh
$ make run
g_sum = 7, g_sub = -1
g_sum = 100, g_sub = 200
```

`add(1,2)` 令 `g_sum=3`、`add(3,4)` 令 `g_sum=7`；

`sub(5,6)`、`sub(7,8)` 令 `g_sub=-1`。

随后 `main` 直接写入 `g_sum=100`、`g_sub=200`，第二次打印验证写入生效。

### 1.2 例子涉及的三种访问

这个例子覆盖了动态链接中最核心的三类符号访问：

| 访问类型                 | 代码位置                      | 目标符号             | 机制             | 解析时机             |
| ------------------------ | ----------------------------- | -------------------- | ---------------- | -------------------- |
| **跨模块函数调用**       | `main` → `add`/`sub`/`printf` | 外部函数             | PLT + GOT.PLT    | 首次调用（延迟绑定） |
| **跨模块全局变量读写**   | `main` 读/写 `g_sum`/`g_sub`  | 共享库里的全局变量   | GOT              | 加载时              |
| **模块内全局变量** | `add` 写 `g_sum`、`sub` 写 `g_sub` | 本模块全局变量 | GOT          | 加载时              |

## 2 动态链接的关键

**弄清楚如下这几张表以及其相互作用是搞懂了动态链接的关键。**

这几张表在不同的阶段（编译时->链接时->加载时->运行时）表中的内容可能会发生变化，所以需要仔细跟踪表中内容的变化。

```sh
$ riscv64-unknown-linux-gnu-readelf -SW dyn_got_plt.elf | grep -E '\.plt|\.got|\.dyn'

  [ 8] .rela.dyn         RELA            0000000000000518 000518 0000f0 18   A  ;
  [ 9] .rela.plt         RELA            0000000000000608 000608 000060 18  AI  ;
  [10] .plt              PROGBITS        0000000000000670 000670 000060 10  AX  ; PLT 桩代码
  [20] .dynamic          DYNAMIC         0000000000001d98 000d98 000220 10  WA  ;
  [21] .got              PROGBITS        0000000000001fb8 000fb8 000038 08  WA  ; 数据符号地址的 GOT
  [22] .got.plt          PROGBITS        0000000000001ff0 000ff0 000030 08  WA  ; PLT 专用的 GOT
```

将上述几张表按照关系分为两部分：

* 跨模块函数调用:  涉及到`.plt`表，`.got.plt`表， 以及`.rela.plt`表

* 跨模块全局变量：涉及到`.got`表，`.rela.dyn`表


### 2.1 跨模块函数调用

**`.plt`、`.got.plt` 和 `.rela.plt` 表是怎么相互配合的？**

编译链接时，链接器为每个外部函数（如 `add`）做三件事：

1. 在 `.plt` 中生成一段短小的桩代码（`add@plt`）；
2. 在 `.got.plt` 中分配一个 8 字节槽位，初始值指向 `.plt` 的公共入口 PLT[0]（即尚未解析）；
3. 在 `.rela.plt` 中记录一条 `R_RISCV_JUMP_SLOT` 重定位条目，指明要把 `add` 的真实地址填到 `.got.plt` 的哪个槽位。

程序运行时：

1. 首次调用 `add`：`main` 跳转至 `add@plt`，该桩从 `.got.plt[add]` 取出地址（仍是 PLT0），进而跳转至  PLT[0]。 PLT[0] 借助 `link_map` 和重定位序号（`.rela.plt`表）知道是要调用add函数，然后调用动态链接器的解析函数找到 `add` 的真实地址后，将其写回 `.got.plt[add]`，然后跳转至 `add` 执行。

2. 后续调用 `add`：`add@plt` 再次从 `.got.plt[add]` 取地址时，拿到的已是 `add` 的真实地址，直接跳转，不再经过动态链接器。

简言之：`.plt` 提供“存根代码”负责从`.got.plt` 取地址并跳转，`.rela.plt` 告诉动态链接器“符号解析完后往哪写”，`.got.plt` 存放“解析结果缓存” 。三者协同实现了函数的延迟绑定——首次调用解析并缓存，后续调用直接跳转。

### 2.2 访问全局变量

**`.rela.dyn` 与 .got表是怎么相互配合的？**

编译链接时，链接器为每个需要动态解析的符号在 `.got` 中预留槽位，同时在 `.rela.dyn` 中记录一条重定位条目（指明“要把符号 X 的地址填到 `.got` 的偏移 Y 处”）。

程序加载时，动态链接器扫描 `.rela.dyn`，逐条解析符号地址，并写入对应的 `.got` 槽位。

运行时，代码通过 `auipc + ld` 从 `.got` 中取出已填好的地址，再通过该地址访问真正的符号。

简言之：`.rela.dyn` 告诉动态链接器“填什么、填到哪”，`.got` 存放“填好的结果”，供运行时代码取用。



下面围绕这几个表展开，看看`编译-> 链接-> 加载 -> 运行`时 这些表分别做了哪些事情。

## 2 编译时：位置无关代码如何"间接"访问

由于共享库 / PIE 的加载地址在运行前不确定，编译器不能把符号的绝对地址写死在指令里，只能生成的PLT桩代码（先到 GOT.PLT 里取地址、再通过地址访问）。

### 2.1 跨模块函数调用：`jal add@plt`

反汇编 `main`（`riscv64-unknown-linux-gnu-objdump -d dyn_got_plt.elf --section=.text`）：

```asm
00000000000006d0 <main>:
 6d0:  1101        addi  sp,sp,-32
 6d2:  4589        li    a1,2
 6d4:  4505        li    a0,1
 6d6:  ec06        sd    ra,24(sp)
 6d8:  e822        sd    s0,16(sp)
 6da:  e426        sd    s1,8(sp)
 6dc:  fc5ff0ef    jal   6a0 <add@plt>       ; add(1,2)
 6e0:  4591        li    a1,4
 6e2:  450d        li    a0,3
 6e4:  fbdff0ef    jal   6a0 <add@plt>       ; add(3,4)
 6e8:  4599        li    a1,6
 6ea:  4515        li    a0,5
 6ec:  fd5ff0ef    jal   6c0 <sub@plt>       ; sub(5,6)
 6f0:  45a1        li    a1,8
 6f2:  451d        li    a0,7
 6f4:  fcdff0ef    jal   6c0 <sub@plt>       ; sub(7,8)
 6f8:  00002417    auipc s0,0x2
 6fc:  8f043403    ld    s0,-1808(s0) # 1fe8 <g_sub@Base>  ; s0 = &g_sub（经 GOT）
 700:  00002497    auipc s1,0x2
 704:  8c84b483    ld    s1,-1848(s1) # 1fc8 <g_sum@Base>  ; s1 = &g_sum（经 GOT）
 708:  4010        lw    a2,0(s0)                           ; a2 = *g_sub
 70a:  408c        lw    a1,0(s1)                           ; a1 = *g_sum
 70c:  00000517    auipc a0,0x0
 710:  0f450513    addi  a0,a0,244 # 800                   ; a0 = 格式串
 714:  f9dff0ef    jal   6b0 <printf@plt>                   ; 第一次打印
 718:  06400593    li    a1,100
 71c:  0c800613    li    a2,200
 720:  00000517    auipc a0,0x0
 724:  0e050513    addi  a0,a0,224 # 800                   ; a0 = 格式串
 728:  c08c        sw    a1,0(s1)                           ; *g_sum = 100（跨模块写）
 72a:  c010        sw    a2,0(s0)                           ; *g_sub = 200（跨模块写）
 72c:  f85ff0ef    jal   6b0 <printf@plt>                   ; 第二次打印
 730:  60e2        ld    ra,24(sp)
 732:  6442        ld    s0,16(sp)
 734:  64a2        ld    s1,8(sp)
 736:  4501        li    a0,0
 738:  6105        addi  sp,sp,32
 73a:  8082        ret
```

**注意：**`main` 里调用的是 `add@plt`、`sub@plt`、`printf@plt`，而不是 `add`/`sub`/`printf` 本身。编译器把对外部函数的调用，都改写为对本模块 PLT 桩的调用。

### 2.2 跨模块访问全局变量：`auipc+ld` 读 GOT，再 `lw/sw`

上面反汇编里，`g_sum`/`g_sub` 的地址各只从 GOT 取一次（存进 `s1`/`s0`），之后读和写都复用：

```asm
 6f8:  auipc s0,0x2
 6fc:  ld    s0,-1808(s0) # 1fe8 <g_sub@Base>   ; 从 GOT['g_sub'](x1fe8) 取出 g_sub 的地址
 700:  auipc s1,0x2
 704:  ld    s1,-1848(s1) # 1fc8 <g_sum@Base>   ; 从 GOT['g_sum'] (0x1fc8 )取出 g_sum 的地址
 708:  lw    a2,0(s0)                            ; 读 *g_sub
 70a:  lw    a1,0(s1)                            ; 读 *g_sum
 ... 打印 ...
 728:  sw    a1,0(s1)                            ; 写 *g_sum = 100
 72a:  sw    a2,0(s0)                            ; 写 *g_sub = 200
```

PIC 访问全局变量的标准做法，两步走：

1. `auipc + ld` 从 GOT 中读出变量的地址（`g_sub` 在 GOT 槽 `0x1fe8`、`g_sum` 在 GOT 槽 `0x1fc8`）；
2. 再用 `lw`/`sw` 通过该地址去读/写变量本身。

> 读和写走的是**同一套机制**（先取地址再解引用），所以跨模块"读"和"写"是等价的，没有本质区别。
>
> 变量地址是"先到 GOT 取地址，再解引用"，跟函数调用"先到 .GOT.PLT 取地址，再跳转"是同一套思想。
>
> 之所以主程序也编译成 `-fPIE -pie`，正是为了让这种跨模块数据访问也走 GOT。若主程序是非 PIE 的普通可执行文件，访问共享库全局变量会退化为复制重定位（copy relocation）——把变量拷贝进主程序自己的 `.bss`，此时主程序和共享库各有一份，不再是同一个变量。用 PIE 可以规避这个陷阱，让访问真正落到共享库的那一份上。

### 2.3 模块内全局变量同样走 GOT

有趣的是：`add` 访问的是自己模块里定义的 `g_sum`，却依然走 GOT，这是因为这个全局变量可以被其他模块定义的同名符号覆盖。链接器在编译期无法确定运行期真正用的是哪一份，所以只能把它们的地址放进 GOT中 。

反汇编 `add.so` 里的 `add`：

```asm
0000000000000490 <add>:
 490:  9d2d        addw  a0,a0,a1
 492:  00002797    auipc a5,0x2
 496:  b467b783    ld    a5,-1210(a5) # 1fd8 <g_sum@@Base-0x34>  ; 从 GOT 取 g_sum 地址
 49a:  c388        sw    a0,0(a5)                                ; 写 g_sum
 49c:  8082        ret
```

反汇编 `sub.so` 里的 `sub`：

```asm
0000000000000488 <sub>:
 488:  9d0d        subw  a0,a0,a1
 48a:  00002797    auipc a5,0x2
 48e:  b5e7b783    ld    a5,-1186(a5) # 1fe8 <g_sub@@Base-0x24>  ; 从 GOT 取 g_sub 地址
 492:  c388        sw    a0,0(a5)                                ; 写 g_sub
 494:  8082        ret
```

同样是 `auipc + ld` 从 GOT 取地址、再 `sw` 写，而不是 `auipc + addi` 直接 PC-relative 算出地址。

原因：`g_sum`/`g_sub` 是 GLOBAL 符号（`readelf -sW` 里 `GLOBAL`），可以被其他模块定义的同名符号覆盖。链接器在编译期无法确定运行期真正用的是哪一份，所以只能把它们的地址放进 GOT，在加载期填真实值。

**注意：**

1. 如果动态库中定义`static`全局变量，其符号是 LOCAL 的、不可覆盖，编译器就能直接 `auipc + addi` 算出地址、不走 GOT。本例为了聚焦 GOT/PLT，这种情况先不讨论。
2. `add.so`与 `main.o`中，`g_sum`在GOT表中的偏移不同，这是因为每一份代码（模块）都有一张自己的 GOT 表，这是位置无关代码（PIC）的基本要求。但动态链接器在加载时，会把它们都指向同一个地方，虽然访问路径不同，但最终抵达的物理地址是同一个。


## 3 链接时：链接器生成 PLT、GOT 和重定位表

### 3.1 外部函数调用

链接器在识别出需要动态链接的外部函数后，会同步地按顺序地为每个函数在 `.plt`、`.got.plt` 和 `.rela.plt` 中各分配一个条目。所以`.plt`、`.got.plt` 和 `.rela.plt` 这3张表的内容一一对应。

**`.rela.plt` 表：**

链接器为每个被调用的外部函数生成一个 `R_RISCV_JUMP_SLOT`重定位，目标位置 `.got.plt` 的偏移分别是 `0x2008`（add）、`0x2010`（printf）、`0x2018`（sub）。

```sh
$ riscv64-unknown-linux-gnu-readelf -rW dyn_got_plt.elf | grep JUMP_SLOT

  Offset  Type                 Symbol
  2000    R_RISCV_JUMP_SLOT    __libc_start_main@GLIBC_2.34
  2008    R_RISCV_JUMP_SLOT    add
  2010    R_RISCV_JUMP_SLOT    printf@GLIBC_2.27
  2018    R_RISCV_JUMP_SLOT    sub
```

**`.plt` 表的初始内容：**

~~~sh
$ riscv64-unknown-linux-gnu-objdump -d -j .plt dyn_got_plt.elf

dyn_got_plt.elf:     file format elf64-littleriscv


Disassembly of section .plt:

0000000000000670 <.plt>:
 670:   00002397                auipc   t2,0x2
 674:   41c30333                sub     t1,t1,t3
 678:   9803be03                ld      t3,-1664(t2) # 1ff0 <.got.plt>
 67c:   fd430313                addi    t1,t1,-44
 680:   98038293                addi    t0,t2,-1664
 684:   00135313                srli    t1,t1,0x1
 688:   0082b283                ld      t0,8(t0)
 68c:   000e0067                jr      t3

0000000000000690 <__libc_start_main@plt>:
 690:   00002e17                auipc   t3,0x2
 694:   970e3e03                ld      t3,-1680(t3) # 2000 <__libc_start_main@GLIBC_2.34>
 698:   000e0367                jalr    t1,t3
 69c:   00000013                nop

00000000000006a0 <add@plt>:
 6a0:   00002e17                auipc   t3,0x2
 6a4:   968e3e03                ld      t3,-1688(t3) # 2008 <add@Base>
 6a8:   000e0367                jalr    t1,t3
 6ac:   00000013                nop

00000000000006b0 <printf@plt>:
 6b0:   00002e17                auipc   t3,0x2
 6b4:   960e3e03                ld      t3,-1696(t3) # 2010 <printf@GLIBC_2.27>
 6b8:   000e0367                jalr    t1,t3
 6bc:   00000013                nop

00000000000006c0 <sub@plt>:
 6c0:   00002e17                auipc   t3,0x2
 6c4:   958e3e03                ld      t3,-1704(t3) # 2018 <sub@Base>
 6c8:   000e0367                jalr    t1,t3
 6cc:   00000013                nop
~~~



**`.got.plt` 表的初始内容：延迟绑定的"暗号"：**

```sh
$ riscv64-unknown-linux-gnu-objdump -s -j .got.plt dyn_got_plt.elf

 1ff0: ffffffff ffffffff 00000000 00000000   ; GOT.PLT[0]=-1, [1]=0
 2000: 70060000 00000000                    ; __libc_start_main -> 0x670
 2008: 70060000 00000000                    ; add              -> 0x670
 2010: 70060000 00000000                    ; printf           -> 0x670
 2018: 70060000 00000000                    ; sub              -> 0x670
```

`0x670` 是 `.plt` 段的起始地址（即 PLT[0]）。

关键：链接时`.got.plt`表所有函数条目的初始值都指向 PLT[0]，而不是真正的函数地址。这是延迟绑定的标志——真正的解析推迟到第一次调用时才做。

### 3.2 访问全局变量 
编译/链接阶段（静态链接器）：

   - 扫描到外部全局变量，在 `.got` 中预留一个 8 字节槽位（初始填 0）。

   - 在 `.rela.dyn` 中新增一条记录，其 `r_offset` 指向刚才预留的 `.got` 槽位地址，`r_info` 指向该变量的符号名。

**`.got` 的初始内容：**

~~~sh
$ riscv64-unknown-linux-gnu-objdump -s -j .got dyn_got_plt.elf
dyn_got_plt.elf:     file format elf64-littleriscv

Contents of section .got:
 1fb8 981d0000 00000000 00000000 00000000  ................
 1fc8 00000000 00000000 00000000 00000000  ................
 1fd8 00000000 00000000 00000000 00000000  ................
 1fe8 00000000 00000000 
~~~

****

`.got` 表的前两项也是特殊项（要注意`.got`表与`.got.plt`表的区分）：

* `.got` 表的 第一个条目（GOT[0]）在 ELF 规范中是一个保留条目，其内容由动态链接器在加载时使用。它通常被设置为指向 **`.dynamic` 节**的地址。

* `.got` 表的 第二个条目（GOT[1]）是保留槽位，通常是空的（`0x00000000`），供动态链接器内部使用或保持对齐。

**`.rela.dyn` 表：**

可执行文件访问 `g_sum`/`g_sub`，链接器在主程序的 `.got` 里为它们各生成一个条目，并产生重定位：

```sh
$ riscv64-unknown-linux-gnu-readelf -rW dyn_got_plt.elf | grep -E 'g_sum|g_sub'

  Offset  Type          Symbol
  1fc8    R_RISCV_64    g_sum
  1fe8    R_RISCV_64    g_sub
```

`add.so` 访问自己的 `g_sum`、`sub.so` 访问自己的 `g_sub`，同样在各自 `.got` 里生成条目。因为每一份代码（模块）都有一张自己的 GOT 表，这是位置无关代码（PIC）的基本要求，同一个全局变量，动态链接器在加载时，会把它们都指向同一个地方，虽然访问路径不同，但最终抵达的物理地址是同一个。

```sh
$ riscv64-unknown-linux-gnu-readelf -rW add.so | grep g_sum

  Offset  Type          Symbol's Value  Symbol + Addend
  1fd8    R_RISCV_64    200c            g_sum + 0     ; g_sum 在 add.so 内偏移 0x200c

$ riscv64-unknown-linux-gnu-readelf -rW sub.so | grep g_sub

  Offset  Type          Symbol's Value  Symbol + Addend
  1fe8    R_RISCV_64    200c            g_sub + 0     ; g_sub 在 sub.so 内偏移 0x200c
```

## 4 加载时：动态链接器的工作

程序启动时，动态链接器（`ld.so`）负责把一切"地址"归位。它做了三类事：

### 4.1 处理位置无关重定位

可执行文件和共享库都有这类重定位，作用是：把"相对基址的偏移"换算成"绝对地址"。

```sh
$ riscv64-unknown-linux-gnu-readelf -rW dyn_got_plt.elf | grep RELATIVE

  Offset  Type                Addend
  1d80    R_RISCV_RELATIVE    75e
  1d88    R_RISCV_RELATIVE    7f0
  1d90    R_RISCV_RELATIVE    7b8
  1fd0    R_RISCV_RELATIVE    6d0    ; .got 里存 main 的地址(0x6d0)
  2020    R_RISCV_RELATIVE    2020   ; .data 段自身
```

以 `1fd0 -> 0x6d0` 为例：动态链接器把 `load_base + 0x6d0` 写入偏移 `0x1fd0`（`main` 的地址）。`_start` 正是从这里取出 `main` 再传给 `__libc_start_main`：

```asm
000000000000073c <_start>:
 73c:  022000ef    jal   75e <load_gp>
 740:  87aa        mv    a5,a0
 742:  00002517    auipc a0,0x2
 746:  88e53503    ld    a0,-1906(a0) # 1fd0 <_GLOBAL_OFFSET_TABLE_+0x18>  ; a0 = *GOT[0x1fd0] = main 的地址
 74a:  6582        ld    a1,0(sp)              ; argc
 74c:  0030        addi  a2,sp,8               ; argv
 74e:  ff017113    andi  sp,sp,-16
 752:  4681        li    a3,0
 754:  4701        li    a4,0
 756:  880a        mv    a6,sp
 758:  f39ff0ef    jal   690 <__libc_start_main@plt>
 75c:  9002        ebreak
```

PIE 之所以连 `main` 的地址都要放进 GOT，是因为 `_start` 自身也是位置无关的，无法在指令里写死 `main` 的绝对地址。

### 4.2 初始化函数调用的GOT.PLT表，为延迟绑定做准备

动态链接器把 `.got.plt` 的前两个特殊条目填好：

```sh
GOT.PLT[0] (0x1ff0) = 解析函数的地址（动态链接器的 resolver）（模块链表指针，初值 -1）
GOT.PLT[1] (0x1ff8) = link_map
```

而 `GOT.PLT[add]`、`GOT.PLT[sub]`、`GOT.PLT[printf]` 保持 0x670 不变，也就是指向 PLT[0]。

### 4.3 填写全局变量的 GOT 表

数据重定位在加载期一次性全部解析完成，不像函数那样延迟，因为数据无法像函数一样"延迟绑定"。

加载/运行阶段（动态链接器）：

- 解析 ELF 文件，找到 `.rela.dyn` 和 `.got`。
- 遍历 `.rela.dyn` 的每条记录，根据 `r_info` 在共享库中查找对应符号的运行时内存地址。
- 将查到的地址，写入 `r_offset` 指定的 `.got` 槽位中。

动态链接器把 `g_sum`、`g_sub` 的运行时地址分别写进主程序和共享库各自的 GOT 条目，这就是前面提到的：同一个全局变量，动态链接器在加载时，会把它们都指向同一个地方，虽然访问路径不同，但最终抵达的物理地址是同一个。

```text
主程序 .got[0x1fc8] = load_base(add.so) + 0x200c   ; g_sum 的真实地址
主程序 .got[0x1fe8] = load_base(sub.so) + 0x200c   ; g_sub 的真实地址
add.so  .got[0x1fd8] = load_base(add.so) + 0x200c  ; g_sum（与主程序那条是同一地址）
sub.so  .got[0x1fe8] = load_base(sub.so) + 0x200c  ; g_sub（与主程序那条是同一地址）
```

加载前这些`.got[]`条目都是 `0`，加载后变成真实地址。

## 5 运行时

### 5.1 函数调用的延迟绑定

**1. PLT0 的桩代码:**

```asm
0000000000000670 <.plt>:          ; PLT0（所有未解析函数的公共入口）
 670:  00002397    auipc t2,0x2
 674:  41c30333    sub   t1,t1,t3       ; t1 = 调用方 PLT 桩的 nop 地址 - t3 = 符号偏移
 678:  9803be03    ld    t3,-1664(t2)   ; t3 = *GOT.PLT[0] = resolver 地址
 67c:  fd430313    addi  t1,t1,-44
 680:  98038293    addi  t0,t2,-1664    ;
 684:  00135313    srli  t1,t1,0x1      ; t1 = reloc_index
 688:  0082b283    ld    t0,8(t0)       ; t0 = *GOT.PLT[1] = link_map
 68c:  000e0067    jr    t3             ; 跳进动态链接器 resolver(link_map, reloc_index)
```

**2. 第一次调用 `add(1, 2)`：绕道动态链接器:**

```asm
main:  jal 6a0 <add@plt>
              │
              ▼
add@plt:
   auipc t3,0x2
   ld    t3,-1688(t3)   ; t3 = GOT.PLT[add] = 0x670（还是 PLT0！）
   jalr  t1,t3          ; 跳到 0x670（PLT0），t1 记下返回位置
              │
              ▼
PLT0:
   ... 算出符号偏移，跳进 resolver
              │
              ▼
动态链接器 resolver：
   1. 用 (link_map, reloc_index) 找到 R_RISCV_JUMP_SLOT 中 "add" 这一项
   2. 解析出 add 在 add.so 中的真实地址
   3. 把真实地址写回 GOT.PLT[add]（0x2008）
   4. 跳转到 add 执行
```

第一次调用要多走一步：PLT 桩 → PLT0 → 动态链接器 → 解析 → 写回 → 执行。

**3. 后续调用 `add(3, 4)`：直接跳转**

第二次调用时，`GOT.PLT[add]` 已经被改写为 `add` 的真实地址：

```asm
main:  jal 6a0 <add@plt>
              │
              ▼
add@plt:
   ld  t3,-1688(t3)   ; t3 = GOT.PLT[add] = add 的真实地址
   jalr t1,t3         ; 直接跳到 add，不再经过动态链接器
```

对比：

```text
第一次： main → add@plt → PLT0 → resolver → add
后续：   main → add@plt → add
```

`sub`、`printf` 同理，每个函数各自独立**、**首次调用时各自解析一次。

### 5.2 跨模块访问全局变量

**跨模块读/写 `g_sum`：加载期已就绪**

`main` 里读和写 `g_sum`/`g_sub` 时：

```asm
   ld    s0, ... # 1fe8   ; s0 = GOT[0x1fe8] = g_sub 的真实地址（§4.3 已填好）
   ld    s1, ... # 1fc8   ; s1 = GOT[0x1fc8] = g_sum 的真实地址
   lw    a2,0(s0)         ; 读 *g_sub
   lw    a1,0(s1)         ; 读 *g_sum
   ...
   sw    a1,0(s1)         ; 写 *g_sum = 100
   sw    a2,0(s0)         ; 写 *g_sub = 200
```

整个过程不再经过动态链接器，因为地址在加载期已经解析完毕。

**模块内写 `g_sum`：加载期已填好 GOT**

`add` 写自己模块里的 `g_sum` 时：

```asm
   auipc a5,0x2
   ld    a5,-1210(a5)   ; 从 add.so 的 GOT[0x1fd8] 读出 g_sum 地址
   sw    a0,0(a5)       ; 写 g_sum
```

同样不经过动态链接器——GOT[0x1fd8] 在加载期（§4.3）已被填成 g_sum 的真实地址，运行期直接取地址、写值。

## 6 观察延迟绑定：`make bind`

前面"延迟绑定"都是静态分析。`LD_DEBUG=bindings` 能让动态链接器打印每次符号解析，把延迟绑定变成可观察的现象。

`Makefile` 里加了 `bind` 目标（`BIND_GREP` 只保留本例关心的符号和 `calling init` 标记）：

```sh
$ make bind
=== 默认（延迟绑定）：add/sub 在 calling init 之后、首次调用时才解析 ===
    binding ... sub.so [0] to ... sub.so [0]: normal symbol `g_sub'     ← 数据：加载期
    binding ... add.so [0] to ... add.so [0]: normal symbol `g_sum'     ← 数据：加载期
    binding ... dyn_got_plt.elf [0] to ... add.so [0]: normal symbol `g_sum'
    binding ... dyn_got_plt.elf [0] to ... sub.so [0]: normal symbol `g_sub'
    calling init: /lib/ld-linux-riscv64-lp64d.so.1
    calling init: /lib64/lp64d/libc.so.6
    calling init: .../sub.so
    calling init: .../add.so
    binding ... dyn_got_plt.elf [0] to ... add.so [0]: normal symbol `add'   ← 运行期！
    binding ... dyn_got_plt.elf [0] to ... sub.so [0]: normal symbol `sub'   ← 运行期！

=== LD_BIND_NOW=1（立即绑定）：add/sub 在 calling init 之前、启动时全部解析 ===
    binding ... sub.so [0] to ... sub.so [0]: normal symbol `g_sub'
    binding ... add.so [0] to ... add.so [0]: normal symbol `g_sum'
    binding ... dyn_got_plt.elf [0] to ... add.so [0]: normal symbol `g_sum'
    binding ... dyn_got_plt.elf [0] to ... sub.so [0]: normal symbol `g_sub'
    binding ... dyn_got_plt.elf [0] to ... add.so [0]: normal symbol `add'   ← 启动时！
    binding ... dyn_got_plt.elf [0] to ... sub.so [0]: normal symbol `sub'   ← 启动时！
    calling init: /lib/ld-linux-riscv64-lp64d.so.1
    calling init: /lib64/lp64d/libc.so.6
    calling init: .../sub.so
    calling init: .../add.so
```

关键差异在于 `add`/`sub` 的解析位置：

- 默认（延迟绑定）：`g_sum`/`g_sub`（数据）在 `calling init` 之前解析；`add`/`sub`（函数）在 `calling init` 之后、即 `main` 执行到首次调用时才解析。
- `LD_BIND_NOW=1`（立即绑定）：所有符号（含 `add`/`sub`）都在 `calling init` 之前、启动阶段一次性解析完。

这从运行期印证了前面的结论：**函数默认延迟绑定，数据加载期绑定**。



## 7 总结

| 访问类型                   | 指令形态                        | 重定位类型          | 解析时机   | 是否延迟       |
| -------------------------- | ------------------------------- | ------------------- | ---------- | -------------- |
| 跨模块函数调用             | `jal foo@plt` → `jalr *GOT.PLT` | `R_RISCV_JUMP_SLOT` | 首次调用时 | 是（延迟绑定） |
| 跨模块全局变量（主程序读写） | `auipc+ld` 读 GOT，再 `lw/sw`   | `R_RISCV_64`        | 加载期     | 否             |
| 模块内全局变量（可抢占）   | `auipc+ld` 读 GOT，再 `lw/sw`   | `R_RISCV_64`        | 加载期     | 否             |
| 位置无关地址               | —                               | `R_RISCV_RELATIVE`  | 加载期     | 否             |

简单来说：

* 可抢占的符号（外部函数、全局变量）才需要 GOT/GOT.PLT 间接访问；
* 函数地址放 .got.plt，首次调用走 PLT 绕道动态链接器（延迟绑定）；
* 变量地址放 .got，加载期一次性解析（不延迟）。