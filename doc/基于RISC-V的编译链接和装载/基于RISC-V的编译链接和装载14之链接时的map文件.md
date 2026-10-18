# 基于RISC-V的编译链接和装载14之链接时的map文件

[toc]

**.map文件** 是由链接器在生成可执行文件时输出的文本文件，用于记录源代码与目标代码的映射关系，包括函数、变量在内存中的地址、大小、所属段等信息。用来帮助开发者调试，分析内存布局、优化存储和排查性能问题。

## 1 编译时怎么生成map文件？

链接时加上`-Wl,-Map`即可生成map文件，加上这个选项后，链接器将所有 `.o` 文件和库文件 “拼” 成一个可执行文件的过程时会生成一个 `.map` 文件

~~~sh
# 链接时使用 -Wl,-Map 选项
$ riscv64-unknown-linux-gnu-gcc -o helloworld helloworld.c -Wl,-Map=helloworld.map
~~~

## 2 map文件有什么用？

 `.map` 文件（链接映射文件）是分析二进制文件内部结构的地图，详细记录了链接器如何将各个目标文件（`.o`）和库文件组合成最终的可执行文件或库。

.map 文件是连接源代码与二进制机器码的桥梁，通过分析 map 文件，可以解决以下几类关键问题：

- 觉得**大**？查它找体积大户；
- 觉得**乱**？查它找符号冲突；
- 觉得**慢**？查它看内存布局和缓存局部性；
- 怕**爆**？查它看段大小是否超限。

### 2.1 代码体积膨胀

这是 map 文件最常见的用途。当你的可执行文件CodeSize很大时，map 文件能帮你找到“元凶”。

- 按模块分析，查看哪个 `.o`文件或哪个静态库（`.a`)贡献了最多的代码
- 按符号分析，查看哪个具体的函数或全局数组占用了最多空间

**举例：**

如下例定义了一个全局数组aaa但是实际并没有使用

~~~c
/* helloworld.c */
#include <stdio.h>

int aaa[1000];
int main()
{
  printf("hello world\n");

  return 0;
}
~~~

编译：

~~~sh
$ riscv64-unknown-linux-gnu-gcc -o helloworld helloworld.c -Wl,-Map=helloworld.map
~~~

从map文件中可以看到aaa数组放到.bss段，起始地址0x12020，占用0xfa0（十进制4000）字节：

~~~~log
 .bss           0x0000000000012020      0xfa0 /tmp/cccVw5cK.o
                0x0000000000012020                aaa
~~~~

可见通过map 文件分析codesize大小很方便。

**代码体积优化：**

针对类似这种定义了但没有实际使用的全局数组，可以使用链接器的“垃圾回收”机制来进行Codesize优化（见第15章）。

简单来说就是编译时加上`-ffunction-sections -fdata-sections`参数，链接时加上`-Wl,--gc-sections`参数：

~~~sh
$ riscv64-unknown-linux-gnu-gcc -o helloworld helloworld.c \
  -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Wl,-Map=helloworld.map
~~~

从map文件可以看到aaa数组出现在`Discarded input sections`，被清除掉了：

~~~log
Discarded input sections
 .bss.aaa       0x0000000000000000      0xfa0 /tmp/cc3v4gHH.o
~~~



### 2.2 查看调用了库中的哪个函数

有时程序能运行，但依赖了意想不到的库。

通过查看 map 文件中引入了哪些库中的符号，可以反向推导出你的代码隐式依赖了哪些库。比如：你以为没用到数学库，但 map 文件显示链接了 `libm` 中的 `sin` 函数。

**Cross Reference Table**（如果链接时加了 `-X` 或 `--cref`）：

这个选项会产生一个表格，列出了每个符号在哪里被定义，以及在哪里被引用。这是排查符号冲突的神器。



### 2.3 用 `.map` 文件定位 “函数未定义” 问题

当链接报错 `multiple definition of 'xxx'` 或者程序行为异常（调用了错误版本的函数）时，map 文件是首选排查工具。

- 查看符号来源，Map 文件列出了每个全局符号最终被解析到了哪个文件的哪个地址；
- 弱符号与强符号：查看弱符号（Weak Symbol）是否被预期的强符号覆盖。



### 2.3 分析内存布局与段分布（Memory Layout）

对于嵌入式开发或对内存敏感的应用，了解代码和数据在内存中的确切位置至关重要。段的大小与位置，Map 文件清晰展示了 `.text` (代码)，`.data`(初始化数据)，`.bss` (未初始化数据)，`.rodata` (只读数据) 等段的起始地址和大小。


  - 嵌入式场景：确认代码是否超出了 Flash 限制，或者 RAM 使用量是否超标；
  - 安全/漏洞分析：确认敏感数据是否被放入了可读写的段，或者代码段是否正确标记为不可写；
- 加载地址验证：验证链接脚本（`.ld` 文件）是否按预期将特定代码放置到了特定的内存区域（如 TCM, OCRAM 等）。



### 2.4 优化启动时间与缓存命中率

- 函数排列顺序，Map 文件显示了函数在 `.text`段中的物理排列顺序

  优化：如果两个经常互相调用的函数在 map 中相距甚远，可能会导致指令缓存（I-Cache）缺失。你可以利用链接器脚本或 `__attribute__((section))` 调整它们的顺序，使热点代码集中在一起；

- 页对齐问题：检查关键数据结构是否跨越了内存页边界，导致额外的 Page Fault。



### 2.6 验证静态链接的完整性

对于静态链接的程序（如前一个问题所述）：

- 确认无动态依赖：检查 map 文件，确保所有需要的符号都解析到了静态库或本地的 `.o` 文件中，没有遗留对动态库的引用（除非是 libc 等基础库的特殊处理）；
- 包含内容审计：确认静态库中哪些模块被实际链接进来了（静态链接器通常只会提取用到的 `.o` 文件），这有助于理解最终二进制的大小构成。



## 3 查看MAP文件的分析工具

GCC 生成的 `.map` 文件动辄几万行、几十万行，里面充斥着各种被编译器 mangling 过的 C++ 符号、无数个被合并的输入文件路径，还有密密麻麻的内存地址，简直就是个“反人类”，每次想查一下到底是哪个 `.o` 文件里的哪个函数把 Flash 或 RAM 给撑爆了，光是用 `Ctrl+F` 搜索和肉眼对齐，就能让人看到眼花。所以这时候还是需要专门的工具才行。



可用`Amap`工具，见参考3。



**注意：**如果map文件中有中文，`Amap`工具并不能解析，这时候需要设置Ubuntu的环境变量：

~~~sh
# 查看当前的语言环境, 是否是中文环境
$ echo $LC_ALL
$ echo $LANG

# 查看支持的语言环境
$ locale -a

# 强制程序使用最基础的、纯 ASCII 的英文（POSIX 标准）环境运行
$ export LC_ALL=C
~~~



**参考：**

1. [(66 封私信 / 80 条消息) 充分理解Linux GCC 链接生成的Map文件 - 知乎](https://zhuanlan.zhihu.com/p/502051758)
1. [新手快速熟悉.map文件-CSDN博客](https://blog.csdn.net/SundayBear/article/details/150530756)
1. [Amap - Sergey Sikorskiy](https://www.sikorskiy.net/info/prj/amap/)