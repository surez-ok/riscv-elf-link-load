# 基于RISC-V的编译链接和装载15之链接器的垃圾回收

[TOC]

默认情况下，链接器会完整载入每个目标文件的所有段，即使段内的代码或数据完全未被调用、没有任何引用，最终仍会被全部打包进可执行文件，造成Flash/RAM空间的浪费。为解决这一问题，GCC/LLVM工具链提供了链接时垃圾回收（Link-time Garbage Collection）机制，可在链接阶段自动识别并剔除未被引用的代码和数据。

## 1. 为什么需要垃圾回收？

在实际嵌入式开发中，常见的情况包括：

- 模块化编程：工程师习惯将不同功能封装在独立的源文件中，即使某个函数在当前项目中未被使用，该文件中的其他有效代码仍会被整体链接；

- 库函数引用：链接静态库时，链接器默认会拉取库中定义的所有符号，即使只使用了库中的一个函数，整个库的代码段都可能被包含；

- 冗余数组/数据结构：开发过程中定义的测试数组、调试用的全局变量，在发布版本中被遗忘，造成固件膨胀。

## 2. GCC垃圾回收的基本原理

GCC的链接时垃圾回收需要编译器与链接器协同工作，分为两个阶段：

### 2.1 编译阶段：生成独立段（`-ffunction-sections -fdata-sections`）

编译器在编译每个源文件时，默认会将所有函数和全局变量放置在同一个段中（如 `.text`、`.data`）。这导致链接器无法以函数/变量为粒度进行回收，只能整段保留。

通过在编译阶段传递以下选项：

```bash
-ffunction-sections   # 每个函数生成独立的 .text.<func_name> 段
-fdata-sections       # 每个全局变量生成独立的 .data.<var_name> 或 .bss.<var_name> 段
```

编译器会将每个函数和全局变量放到**独立的段**中。例如：

| **原始编译**                             | **启用 `-ffunction-sections`**                             |
| ---------------------------------------- | ---------------------------------------------------------- |
| `func_a()` 和 `func_b()` 都在 `.text` 段 | `func_a()` → `.text.func_a`<br>`func_b()` → `.text.func_b` |
| `var_x` 和 `var_y` 都在 `.data` 段       | `var_x` → `.data.var_x`<br>`var_y` → `.data.var_y`         |

这样，链接器在后续阶段就可以以节（Section）为最小单元，判断每个节是否被引用。

### 2.2 链接阶段：执行垃圾回收（`-Wl,--gc-sections`）

在链接阶段，通过以下选项启用垃圾回收：

```bash
-Wl,--gc-sections    # 链接器执行 GC，剔除未被引用的段
```

链接器会执行如下步骤：

1. 构建引用图（Reference Graph）：从入口点（Entry Point）开始，遍历所有可能被调用的函数和访问的数据，构建一个依赖关系图。
2. 标记存活节（Mark Live Sections）：从入口点出发，递归标记所有可达的节为“存活”。
3. 剔除死节（Sweep Dead Sections）：将所有未被标记的节从最终的可执行文件中移除。

### 2.3 完整编译命令示例

第14章第2.1节举了一个定义了较大的数组，并且后面没有用到，导致ELF体积膨胀，所以可以通过垃圾回收来解决这个问题

## 3. 使用 `-ffunction-sections -fdata-sections` 的注意事项

### 3.1 对链接脚本的影响

启用独立段后，链接脚本需要使用通配符来匹配所有生成的段，否则这些段可能无法被正确链接：

```ld
SECTIONS
{
    .text : 
    {
        *(.text)            /* 匹配默认 .text 段 */
        *(.text.*)          /* 匹配所有 .text.func_a、.text.func_b 等 */
    } > FLASH
    
    .data : 
    {
        *(.data)
        *(.data.*)          /* 匹配所有 .data.var_x 等 */
    } > RAM AT> FLASH
    
    .bss : 
    {
        *(.bss)
        *(.bss.*)           /* 匹配所有 .bss.var_y 等 */
    } > RAM
}
```

### 3.2 对 `-gc-sections` 的补充规则

在某些场景下，即使一个段未被直接引用，也需要强制保留。此时可以在链接脚本中使用 `KEEP` 关键字：

```ld
SECTIONS
{
    .text : 
    {
        KEEP(*(.isr_vector))   /* 强制保留中断向量表 */
        KEEP(*(.init))         /* 强制保留初始化代码 */
        *(.text)
        *(.text.*)
    } > FLASH
}
```

## 4. 垃圾回收的局限性

**1. 间接调用（Indirect Call）：**

通过函数指针、虚函数表（vtable）等方式的间接调用，链接器无法静态分析出调用关系，可能误判函数为“未使用”并将其剔除。

**解决方案**：

- 在链接脚本中使用 `KEEP` 强制保留所有可能的间接调用目标。
- 或者在C代码中通过 `__attribute__((used))` 显式标记：

```c
__attribute__((used)) void callback_handler(void) 
{
    // 通过函数指针间接调用
}
```

**2 弱符号（Weak Symbol）：**

弱符号（`__attribute__((weak))`）如果未被强符号覆盖，链接器可能将其视为未引用而剔除，导致运行时调用弱符号默认实现时发生异常。

**解决方案**：在链接脚本中使用 `KEEP` 强制保留弱符号的定义。

**3 跨模块的内联函数：**

内联函数（`inline`）在编译时会被展开到调用处，可能不再作为独立的节存在，从而绕过 GC 机制。但在某些情况下（如 `-Os` 优化），编译器可能选择不展开内联函数，此时仍会被 `-ffunction-sections` 处理。

## 5. 验证垃圾回收效果

**查看节信息：**

```bash
# 查看目标文件的节列表
$ riscv64-unknown-elf-objdump -h main.o | grep .text

# 查看最终 ELF 文件的节列表（验证是否被剔除）
$ riscv64-unknown-elf-objdump -h firmware.elf | grep .text.func_b
```

**对比固件体积：**

```bash
# 使用 size 命令查看各段大小
$ riscv64-unknown-elf-size firmware.elf
```

**使用 Map 文件分析：**

链接器生成的 Map 文件（`-Map=firmware.map`）详细记录了每个段的最终分配情况，可用于分析哪些段被保留、哪些被剔除：

```bash
$ riscv64-unknown-elf-gcc -Wl,-Map=firmware.map -Wl,--gc-sections ...
$ grep "REMOVE" firmware.map   # 查看被移除的段
```

## 6. 常用场景

| **阶段** | **推荐选项**                          | **作用**                                |
| -------- | ------------------------------------- | --------------------------------------- |
| 编译     | `-ffunction-sections -fdata-sections` | 每个函数/变量生成独立段，为 GC 准备     |
| 链接     | `-Wl,--gc-sections`                   | 链接时剔除未被引用的段                  |
| 链接脚本 | `KEEP` 关键字                         | 强制保留入口点、中断向量表等必须段      |
| C代码    | `__attribute__((used))`               | 标记必须保留的函数/变量（间接调用场景） |
| 发布     | `-Wl,--strip-all` 或 `-s`             | 进一步剥离符号表，减小固件体积          |



链接器的垃圾回收（`--gc-sections`）是嵌入式开发中缩小固件体积、提高内存利用率的核心手段之一。它依赖编译器先通过 `-ffunction-sections -fdata-sections` 将每个函数/变量独立成段，然后链接器从入口点出发构建引用图，剔除所有未被引用的段。实际使用中，需要注意间接调用、弱符号等场景可能导致误剔除，应配合 `KEEP` 和 `__attribute__((used))` 进行显式保护。

在大型嵌入式项目中，开启 GC 通常能减少 10%~30% 的固件体积，尤其在使用静态库时效果更为显著。



**参考：**

1. [RISC-V Binutils工具链02之gcc工具的使用](https://www.cnblogs.com/sureZ-learning/p/22754640)