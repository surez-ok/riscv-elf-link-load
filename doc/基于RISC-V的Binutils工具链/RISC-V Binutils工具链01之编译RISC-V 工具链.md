# RISC-V Binutils工具链01之编译RISC-V 工具链

[TOC]

如果不理解软件的编译流程，可能面对一些奇怪编译问题束手无策，所以了解GCC的编译过程以及GCC工具链的使用是嵌入式底层软件工程师必备的技能。

这个系列准备讲述RISC-V Linux  GCC 工具链，列举这些工具的常用场景，便于查阅。

文中使用了一些测试用例用来辅助讲解，见github：https://github.com/surez-ok/riscv-elf-link-load.git

测试环境：Ubuntu 22.04 x86



系列文章如下：

* [RISC-V Binutils工具链01之编译RISC-V 工具链](RISC-V Binutils工具链01之编译RISC-V 工具链.md)
* [RISC-V Binutils工具链02之gcc工具的使用](RISC-V Binutils工具链02之gcc工具的使用.md)
* [RISC-V Binutils工具链03之readelf工具的使用](RISC-V Binutils工具链03之readelf工具的使用.md)
* [RISC-V Binutils工具链04之ld工具的使用](RISC-V Binutils工具链04之ld工具的使用.md)
* [RISC-V Binutils工具链05之objdump工具的使用](RISC-V Binutils工具链05之objdump工具的使用.md)
* [RISC-V Binutils工具链06之objcopy工具的使用](RISC-V Binutils工具链06之objcopy工具的使用.md)
*  [RISC-V Binutils工具链07之ar工具的使用](RISC-V Binutils工具链07之ar工具的使用.md)
*  [RISC-V Binutils工具链08之as工具的使用](RISC-V Binutils工具链08之as工具的使用.md)
* [RISC-V Binutils工具链09之nm工具的使用](RISC-V Binutils工具链09之nm工具的使用.md)
* [RISC-V Binutils工具链10之size工具的使用](RISC-V Binutils工具链10之size工具的使用.md)
* [RISC-V Binutils工具链11之strip工具的使用](RISC-V Binutils工具链11之strip工具的使用.md)
* [RISC-V Binutils工具链12之strings工具的使用](RISC-V Binutils工具链12之strings工具的使用.md)
* [RISC-V Binutils工具链13之addr2line工具的使用](RISC-V Binutils工具链13之addr2line工具的使用.md)
* [RISC-V Binutils工具链14之c++filt工具的使用](RISC-V Binutils工具链14之c++filt工具的使用.md)
* [RISC-V Binutils工具链15之file命令的使用](RISC-V Binutils工具链15之file命令的使用.md)
* [RISC-V Binutils工具链16之ldd命令的使用](RISC-V Binutils工具链16之ldd命令的使用.md)
* [RISC-V Binutils工具链17之make命令的使用](RISC-V Binutils工具链17之make命令的使用.md)
* [RISC-V Binutils工具链18之xxd命令的使用](RISC-V Binutils工具链18之xxd命令的使用.md)

## 1 编译RISC-V 交叉工具链

子曰：“工欲善其事，必先利其器”，所以首先我们要获得RISC-V工具链：

**方法1：**下载预编译好的工具链（见参考1仓库的[Release]([Releases · riscv-collab/riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain/releases))，下载后设置PATH环境变量，即可使用）；

**方法2：**从源码编译RISC-V的工具链，包括GCC/LLVM与Binutils等工具



这两种方式获取RISC-V工具链都可以，下面讲述源码编译工具链。

**源码编译RISC-V工具链：**

构建系统：Ubuntu20.04，构建时间：20250702 （riscv-gnu-toolchain 项目在持续发展中，所以不确定文中的编译方式会一直有效，最好的方法是直接查看[riscv-gnu-toolchain README](https://github.com/riscv-collab/riscv-gnu-toolchain/blob/master/README.md)）

项目提供两种构建模式：

* Newlib工具链（裸机ELF），工具链前缀`riscv64-unknown-elf-`
* Glibc工具链（Linux ELF），工具链前缀`riscv64-unknown-linux-gnu-`

### 1.1 GCC与Binutils的关系

GCC（GNU Compiler Collection）和 Binutils（Binary Utilities）是 GNU 工具链中的两个核心组件，它们在编译、链接和构建可执行程序的过程中紧密配合，各自承担不同的职责。

| 项目         | GCC                                                          | Binutils                                                     |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **功能**     | 源代码 → 汇编代码 → 目标文件（.o）                           | 处理目标文件、链接、反汇编等                                 |
| **主要组件** | 编译器（cc1）等                                              | `as`（汇编器）、`ld`（链接器）、`objdump`、`nm`、`readelf`、`strip`、`ar`、`ranlib` 等 |
| **作用**     | 编译源代码                                                   | 汇编、链接、分析、处理目标文件                               |
| **关系**     | GCC 依赖 Binutils 提供的汇编器和链接器（GCC需使用Binutils 中的 `as` 和 `ld`） | Binutils 不依赖 GCC， Binutils为 GCC 提供底层工具支持        |

简单来说：GCC 依赖 Binutils 提供的汇编器和链接器，所以编译GCC时需要先编译Binutils，GCC 是“大脑”，Binutils 是“手脚”。GCC 负责理解源代码，Binutils 负责处理底层的机器码和目标文件格式。

### 1.2 编译得到RISC-V 工具链

编译RISC-V 工具链的方法如下：

**step1： 下载代码**

~~~sh
$ git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git
$ cd riscv-gnu-toolchain
$ git submodule update --init --recursive
~~~

**注意：**有些模块编译拉取失败，如果不影响的话，可以删掉。例如：拉取dejagnu模块失败：

例如，某次拉取dejagnu模块失败日志如下：

~~~log
Submodule 'dejagnu' (https://git.savannah.gnu.org/git/dejagnu.git) registered for path 'dejagnu'
Cloning into '/study/riscv-gnu-toolchain/dejagnu'...
fatal: unable to access 'https://git.savannah.gnu.org/git/dejagnu.git/': gnutls_handshake() failed: The TLS connection was non-properly terminated.
~~~

dejagnu模块是测试框架，对我们编译工具链没有影响，可以删掉。执行如下命令删掉dejagnu模块，重新执行update即可：

~~~sh
$ git rm dejagnu
$ git submodule update --init --recursive
~~~

**step2: 安装依赖：**

安装编译依赖，以Ubuntu22.04为例，其它平台请参考[riscv-gnu-toolchain README](https://github.com/riscv-collab/riscv-gnu-toolchain/blob/master/README.md)

~~~sh
$ sudo apt update
$ sudo apt install -y autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev
~~~

**step3: 编译Newlib GCC 工具链：**

~~~sh
$ ./configure --prefix=$(pwd)/build_newlib
$ make -j$(nproc)
~~~

编译multilib newlib gcc（同时支持rv32与rv64）

~~~sh
$ ./configure --prefix=$(pwd)/build_newlib --enable-multilib
$ make -j$(nproc)
~~~

**step4: 编译Glibc GCC 工具链：**

编译Glibc GCC工具链需要`make linux`

~~~sh
$ ./configure --prefix=$(pwd)/build_glibc
$ make linux -j$(nproc)
~~~

编译multilib glibc gcc（同时支持rv32与rv64）

~~~sh
$ ./configure --prefix=$(pwd)/build_glibc --enable-multilib
$ make linux -j$(nproc)
~~~

也编译支持自定义的multilib（即不同march mabi 组合）的newlib gcc与glib gcc，细节请参考[riscv-gnu-toolchain README](https://github.com/riscv-collab/riscv-gnu-toolchain/blob/master/README.md)

**编译RISC-V 工具链的一般步骤：**

1. 编译 Binutils（生成 as、ld 等）
2. 编译 GCC（第一阶段，仅编译 C 编译器）
3. 编译C 库（ Newlib或Glibc库）
4. 重新编译 GCC（第二阶段，支持 C++、Fortran 等）

**编译结果为：**

| Newlib 工具链                     | Glibc 工具链                                   | 描述或文章链接                                               |
| --------------------------------- | ---------------------------------------------- | ------------------------------------------------------------ |
| riscv64-unknown-elf-addr2line     | riscv64-unknown-linux-gnu-addr2line            | [RISC-V Binutils工具链13之addr2line工具的使用](RISC-V Binutils工具链13之addr2line工具的使用.md) |
| riscv64-unknown-elf-ar            | riscv64-unknown-linux-gnu-ar                   | [RISC-V Binutils工具链07之ar工具的使用](RISC-V Binutils工具链07之ar工具的使用.md) |
| riscv64-unknown-elf-as            | riscv64-unknown-linux-gnu-as                   | [RISC-V Binutils工具链08之as工具的使用](RISC-V Binutils工具链08之as工具的使用.md) |
| riscv64-unknown-elf-c++filt       | riscv64-unknown-linux-gnu-c++filt              | [RISC-V Binutils工具链14之c++filt工具的使用](RISC-V Binutils工具链14之c++filt工具的使用.md) |
| riscv64-unknown-elf-elfedit       | riscv64-unknown-linux-gnu-elfedit              | 修改 ELF 文件头部字段的工具，不改变程序内容，仅更新元信息（如架构、OS 类型、文件类型等 |
| riscv64-unknown-elf-g++           | riscv64-unknown-linux-gnu-g++                  | C++编译器                                                    |
| riscv64-unknown-elf-gcc           | riscv64-unknown-linux-gnu-gcc                  | [RISC-V Binutils工具链02之gcc工具的使用](RISC-V Binutils工具链02之gcc工具的使用.md) |
| riscv64-unknown-elf-gcov          | riscv64-unknown-linux-gnu-gcov                 | 代码覆盖率分析工具                                           |
| riscv64-unknown-elf-gcov-dump     | riscv64-unknown-linux-gnu-gcov-dump            | 分析 `.gcda` 和 `.gcno` 文件的内部结构                       |
| riscv64-unknown-elf-gcov-tool     | riscv64-unknown-linux-gnu-gcov-tool            | `gcov` 工具链中的“覆盖率数据管家”                            |
| riscv64-unknown-elf-gdb           | riscv64-unknown-linux-gnu-gdb                  | [GDB之常用命令介绍](https://www.cnblogs.com/sureZ-learning/p/17322603.html) |
| riscv64-unknown-elf-gdb-add-index | riscv64-unknown-linux-gnu-gdb-add-index        | 为静态库生成 GDB 调试符号索引，提升调试体验                  |
| riscv64-unknown-elf-gprof         | riscv64-unknown-linux-gnu-gprof                | 性能分析工具                                                 |
| riscv64-unknown-elf-gstack        | riscv64-unknown-linux-gnu-gstack               | bash脚本，用于分析 RISC-V 架构程序的栈回溯（stack backtrace）信息 |
| riscv64-unknown-elf-ld            | riscv64-unknown-linux-gnu-ld                   | [RISC-V Binutils工具链04之ld工具的使用](RISC-V Binutils工具链04之ld工具的使用.md) |
| riscv64-unknown-elf-ld.bfd        | riscv64-unknown-linux-gnu-ld.bfd               | 实际的RISC-V 工具链中的链接器                                |
| riscv64-unknown-elf-lto-dump      | riscv64-unknown-linux-gnu-lto-dump             | 用于分析和转储 LTO（Link Time Optimization，链接时优化）中间代码的调试工具 |
| riscv64-unknown-elf-nm            | riscv64-unknown-linux-gnu-nm                   | [RISC-V Binutils工具链09之nm工具的使用](RISC-V Binutils工具链09之nm工具的使用.md) |
| riscv64-unknown-elf-objcopy       | riscv64-unknown-linux-gnu-objcopy              | [RISC-V Binutils工具链06之objcopy工具的使用](RISC-V Binutils工具链06之objcopy工具的使用.md) |
| riscv64-unknown-elf-objdump       | riscv64-unknown-linux-gnu-objdump              | [RISC-V Binutils工具链05之objdump工具的使用](RISC-V Binutils工具链05之objdump工具的使用.md) |
| riscv64-unknown-elf-ranlib        | riscv64-unknown-linux-gnu-ranlib               | 生成或更新静态库（.a 文件）索引，优化静态库的链接效率，使用`ar -s` 可以调用这个工具 |
| riscv64-unknown-elf-readelf       | riscv64-unknown-linux-gnu-readelf              | [RISC-V Binutils工具链03之readelf工具的使用](RISC-V Binutils工具链03之readelf工具的使用.md) |
| riscv64-unknown-elf-run           | riscv64-unknown-linux-gnu-run                  | RISC-V 架构的指令级模拟器                                    |
| riscv64-unknown-elf-size          | riscv64-unknown-linux-gnu-size                 | [RISC-V Binutils工具链10之size工具的使用](RISC-V Binutils工具链10之size工具的使用.md) |
| riscv64-unknown-elf-strings       | riscv64-unknown-linux-gnu-strings              | [RISC-V Binutils工具链12之strings工具的使用](RISC-V Binutils工具链12之strings工具的使用.md) |
| riscv64-unknown-elf-strip         | riscv64-unknown-linux-gnu-strip                | [RISC-V Binutils工具链11之strip工具的使用](RISC-V Binutils工具链11之strip工具的使用.md) |
|                                   | riscv64-unknown-linux-gnu-gprofng              | 下一代性能分析工具，是传统 gprof工具的现代化替代品           |
|                                   | riscv64-unknown-linux-gnu-gprofng-archive      | 与性能分析工具 gprofng配套的归档工具                         |
|                                   | riscv64-unknown-linux-gnu-gprofng-collect-app  | gprofng性能分析套件的一部分                                  |
|                                   | riscv64-unknown-linux-gnu-gprofng-display-html | gprofng性能分析套件的可视化工具                              |
|                                   | riscv64-unknown-linux-gnu-gprofng-display-src  | gprofng性能分析套件的源码级分析工具                          |
|                                   | riscv64-unknown-linux-gnu-gprofng-display-text | gprofng性能分析套件的文本报告生成工具                        |

后面将对这些工具链中的大部分进行介绍（见表中链接文章），以Glibc工具链（即前缀为`riscv64-unknown-linux-gnu-`）为例介绍。

**注意：**Newlibc工具链（前缀为riscv64-unknown-elf-）也有相同用法，只是库不同，就不再重复介绍了。

**step5: 编译QEMU**

qemu支持如下目标文件：

1. 用户态模拟（Linux User Mode）

   | 编译目标           | 生成可执行文件 | 功能说明                                                 |
   | ------------------ | -------------- | -------------------------------------------------------- |
   | riscv64-linux-user | `qemu-riscv64` | 在宿主系统（如x86）上直接运行RISC-V 64位Linux 用户态程序 |
   | riscv32-linux-user | `qemu-riscv32` | 在宿主系统上直接运行 RISC-V 32位Linux 用户态程序         |

2. 全系统模拟（System Mode）

   | 编译目标        | 生成可执行文件        | 功能说明                                                     |
   | --------------- | --------------------- | ------------------------------------------------------------ |
   | riscv64-softmmu | `qemu-system-riscv64` | 模拟完整的 RISC-V 64位硬件系统（CPU+外设），可运行Linux/裸机程序 |
   | riscv32-softmmu | `qemu-system-riscv32` | 模拟完整的 RISC-V 32位硬件系统                               |

以编译rv64用户态以及系统态qemu为例，编译方法如下：

~~~sh
$ cd riscv-gnu-toolchain/qemu
$ mkdir build && cd build
$ ../configure --prefix=$(pwd)/linux_qemu --disable-pa --disable-linux-aio --disable-rbd --disable-vnc --disable-sdl --disable-sdl-image --target-list=riscv64-linux-user,riscv64-softmmu
$ make -j$(nproc)
~~~

在build/linux_qemu 路径下可以看到生成的qemu-riscv64与qemu-system-riscv64

## 2 编译程序

### 2.1 分步骤编译

编译如下c代码：

~~~c
/* helloworld.c */
#include <stdio.h>

int main()
{
  printf("hello, world\n");
  return 0;
}
~~~

GCC 会依次调用以下工具：

1. **预处理（Preprocessing）**
   预处理器，处理宏定义、头文件包含等

   ~~~sh
   $ riscv64-unknown-linux-gnu-gcc -E helloworld.c -o helloworld.i
   ~~~

2. **编译（Compilation）**
    cc1：GCC 内部编译器前端，将 `.c` 文件编译为 `.s` 汇编代码

   ~~~sh
   $ riscv64-unknown-linux-gnu-gcc -S -O2 helloworld.i -o helloworld.s
   ~~~

3. **汇编（Assembly）**
   as：将 `.s` 汇编代码转换为 `.o` 目标文件（由 Binutils 提供）

   ~~~sh
   $ riscv64-unknown-linux-gnu-gcc -c helloworld.s -o helloworld.o
   ~~~

4. **链接（Linking）**
    ld：将多个 `.o` 文件和库链接为最终的可执行文件（由 Binutils 提供）

   ~~~sh
   $ riscv64-unknown-linux-gnu-gcc helloworld.o -o helloworld.elf
   ~~~

### 2.2 一步到位的编译

上面展示的是分布编译，也可以使用下面一条命令完成整个编译，并传入了-march -mabi -O2 -g选项：

~~~sh
# 默认是动态链接的可执行文件
$ riscv64-unknown-linux-gnu-gcc -march=rv64imafdc -mabi=lp64d -O2 -g helloworld.c -o helloworld.elf

# 或者编译静态的可执行文件
$ riscv64-unknown-linux-gnu-gcc -march=rv64imafdc -mabi=lp64d -O2 -static -g helloworld.c -o helloworld_static.elf
~~~

### 2.3 在qemu上测试【可选】

在qemu中运行这个elf[可选]：

~~~sh
# 动态程序需指定 sysroot，目的是避免混用宿主系统（如 x86）的头文件和库
$ qemu-riscv64 -L /path/to/sysroot -cpu rv64,g=true,c=true,v=true ./helloworld.elf
# 静态程序直接运行
$ qemu-riscv64 -cpu rv64,g=true,c=true,v=true ./helloworld_static.elf
~~~

后面的文章用到的可执行文件都就是这里生成的ELF。

### 2.4 使用Makefile自动编译

由上述步骤可以看出，分步骤编译比较繁琐，如果一个大型项目，代码量往往动辄几十万甚至上百万行，包含成百上千个源文件和错综复杂的头文件依赖，通过手工一步步编译，完全不可能，所以我们有了make与Makefile工具，目的就是为了实现编译过程的自动化。

见仓库中的helloworld用例：

~~~sh
$ git clone https://github.com/surez-ok/riscv-elf-link-load.git
$ cd riscv-elf-link-load/test/01_helloworld
$ make
~~~

编译得到静态可执行程序：`helloworld_static.elf`

如果希望编译动态链接的`helloworld.elf`，需要修改Makefile，将`STATIC_FLAGS`变量置空，重新make即可。



**参考：**

1. https://github.com/riscv-collab/riscv-gnu-toolchain
3. [GNU Binutils 全工具指南：从编译到逆向的完整生态-CSDN博客](https://blog.csdn.net/knowledgebao/article/details/146052612)
4. https://www.xrvm.cn/document?temp=agn3g5&slug=xuantie-cpu-userguide

