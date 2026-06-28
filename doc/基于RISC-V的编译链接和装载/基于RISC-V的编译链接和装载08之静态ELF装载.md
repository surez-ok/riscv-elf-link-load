# 基于RISC-V的编译链接和装载08之静态ELF装载

[TOC]

静态ELF的装载过程很简单，简单来说就是：**“准备好栈空间，然后跳转到函数入口地址”**，就可以向下执行了。

如果讨论得更细致，需要区分裸机环境下ELF装载以及linux系统环境下ELF装载。

## 1 裸机环境下ELF装载

对于裸机代码，只能使用静态ELF，裸机环境下静态ELF装载的核心就是：

* 设置栈指针，为函数调用和局部变量的存储准备好栈
* 跳转到入口，开始执行C代码

可以查看RISC-V 裸机启动流程，嵌入式系统上电后执行的第一段软件代码是引导程序，这段程序往往由汇编语言编写(一般为`crt0.S` 或 `Start.S`)。

简略描述如下，可见设置栈指针以及跳转入口是其中的核心步骤。

~~~log
1. CPU复位
   ↓
2. PC跳转到复位向量
   ↓
3. 设置栈指针
   ↓
4. 清零.bss段
   ↓
5. 复制.data段 (ROM → RAM)
   ↓
6. 设置中断/异常向量
   ↓
7. 初始化C库 (可选)
   ↓
8. 跳转到main()
~~~



**为什么裸机环境一定要设置栈指针呢？**

更准确的是问：为什么C函数一定要提前设置好栈指针呢？

答案是：因为RISC-V寄存器有限，需要来回在这些寄存器里面倒腾数据，所以在进入函数时候将寄存器的值保存到栈里，函数返回时将环境恢复如初。



## 2 linux环境下ELF装载

### 2.1 Linux 环境ELF的装载执行流程
后面会和动态ELF装载一起介绍，见第11章。

### 2.2 一个简单的ELF加载器

常见的ELF加载器的设计思路：

首先通过读取ELF文件头来提取出信息，然后使用read系统调用将各个段读取到内存中，并将其存储在ELF Exec结构体的相应成员中。最后，跳转到ELF Exec的entry指向的位置开始执行程序。

`03_simple_elfloader`用例是一个简单的用户态（userspace）的 RISC-V ELF 加载器，模仿 Linux 内核的 `fs/binfmt_elf.c`。它把 ELF 文件映射进当前进程的地址空间、构造一份 Linux 规范的初始栈（argc/argv/envp/auxv），然后跳转到入口点。动态链接的 ELF 交给 [ld.so](http://ld.so/) 去完成链接，静态 PIE 则直接跳程序入口。

详细的编译参数见用例中的README：

~~~sh
$ cd riscv-elf-link-load/test/03_simple_elfloader
$ make all        # build RV64  -> build/rv64/
$ make rv32       # build RV32  -> build/rv32/
$ make run        # build + run RV64 tests under qemu
$ make rv32-run   # build + run RV32 tests under qemu
~~~

其整体流程为：

~~~log
打开 ELF 文件
  └─ read_elf_headers()     读并校验 ELF 头 + program header 表
  └─ load_elf()             加载主程序（目标 ELF， 把PT_LOAD 段映射进内存）
  └─ 扫描 PT_INTERP：
       └─ 有 → read_elf_headers + load_elf 加载 ld.so
       └─ 无 → 静态程序，跳过
  └─ mmap 一块 8MB 新栈
  └─ build_initial_stack()  构造初始栈
  └─ jump_to()              内联汇编：mv sp; jr entry，永不返回
~~~

这个用例支持两种目标：

1. 动态 ELF（`ET_DYN + PT_INTERP`）：如 `hello_dyn` → 加载 [ld.so](http://ld.so/)，跳 [ld.so](http://ld.so/) 入口，由它完成重定位后跳 `main`。
2. 静态 PIE（`ET_DYN`，无 `PT_INTERP`）：如 `hello_static`（`-static-pie`）→ 直接跳程序入口。

动态ELF加载后续第11章会继续讲述。



**参考：**

1. https://blog.51cto.com/u_11529070/9213902
2. [【内核】ELF 文件执行流程 - _hong - 博客园](https://www.cnblogs.com/lianyihong/p/17911643.html)
3. [embedded2014/elf-loader: Simplified ELF loader for ARM Cortex-M4 based platform](https://github.com/embedded2014/elf-loader)
3. https://github.com/moshego189/tiny-link
3. [martinribelotta/elfloader: ARMv7M ELF loader](https://github.com/martinribelotta/elfloader)
3. https://github.com/xrw67/elfloader
3. [riscv-isa-sim/fesvr/elfloader.cc at master · riscv-software-src/riscv-isa-sim](https://github.com/riscv-software-src/riscv-isa-sim/blob/master/fesvr/elfloader.cc)
3. https://github.com/riscv-software-src/riscv-pk

