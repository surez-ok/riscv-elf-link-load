# RISC-V Binutils工具链14之c++filt工具的使用

[TOC]

c++filt是一个符号解修饰工具，用于将编译器生成的 **“mangled”（修饰）符号名** 还原为人类可读的原始函数名、类名、模板等。  
常见于分析 `g++`、`clang++` 编译后的符号表、崩溃日志、`nm`/`objdump` 输出等。

## 1 c++filt 工具常用选项

~~~sh
$ riscv64-unknown-linux-gnu-c++filt --help
~~~

输出log见：[riscv64-unknown-linux-gnu-c++filt_help](images/riscv64-unknown-linux-gnu-c++filt_help.log)

## 2 c++filt基本使用方法

C++ 支持函数重载、命名空间、类、模板等特性，所以几个函数可能具有相同的名称但入参类型不同。为了在编译后的二进制文件中唯一标识这些函数，编译器会对函数名进行编码（这个过程叫做**名字修饰**），编码后的函数名不易阅读，查看反汇编很困难，所以需要工具对编码后的函数名进行反修饰（ **demangling**）。

`c++filt` 的作用就是将这些被修饰的符号名“解码”回原始的、可读的 C++ 函数名。注意：`c++filt` 默认情况下不能进行 C++ 名字编码（mangling），它是一个单向的解码（demangling）工具。

例如：如下opencv的反汇编片段，其中`_ZN2cv6imreadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi`经过了名字修饰，不易阅读

~~~dasm
1fa92:       12010d13                addi    s10,sp,288
1fa96:       856a                    mv      a0,s10
1fa98:       4601                    li      a2,0
1fa9a:       85a2                    mv      a1,s0
1fa9c:       1da090ef                jal     28c76 <_ZN2cv6imreadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi>
1faa0:       8522                    mv      a0,s0
1faa2:       00ef6097                auipc   ra,0xef6
~~~

可以使用c++filt来进行处理：

~~~sh
$ riscv64-unknown-linux-gnu-c++filt _ZN2cv6imreadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi
~~~

结果为：

~~~log
cv::imread(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, int)
~~~

即为cv::imread函数

## 3 c++filt更多功能

**解码多个符号：**

~~~sh
$ riscv64-unknown-linux-gnu-c++filt _Z4funci _Z4funcd _ZN5MyClassC1Ev
~~~

**结合其他工具使用：**


查看目标文件中的符号，并解码 C++ 名字
~~~sh
$ riscv64-unknown-linux-gnu-nm opencv.o | riscv64-unknown-linux-gnu-c++filt
~~~

## 4 网页反修饰工具

也可以使用在线 demangler 工具，输入符号即可看到结果

[https://demangler.com/](https://demangler.com/)

