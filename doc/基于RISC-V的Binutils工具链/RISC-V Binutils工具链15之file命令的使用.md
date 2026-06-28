# RISC-V Binutils工具链15之file命令的使用

[TOC]

file 命令用于识别文件的类型。

它不会简单地通过文件扩展名（如 `.txt`, `.jpg`）来判断，而是通过读取文件内容的“魔数”（magic number）和结构特征，来确定文件的真实类型。

## 1 file 工具常用选项

~~~sh
$ file --help
~~~

输出log见：[file_help](images/file_help.log)

## 2 典型使用场景

### 2.1  判断文件的真实类型（不依赖扩展名）

即使文件被改名或没有扩展名，`file` 也能识别。

```bash
$ file image.png
image.png: PNG image data, 800 x 600, 8-bit/color RGB, non-interlaced

$ file document.pdf
document.pdf: PDF document, version 1.7

$ file video.mp4
video.mp4: ISO Media, MP4 v2 [H.264] ...
```

即使你把一个 `.png` 图片重命名为 `abc.txt`，`file` 依然能识别它是 PNG

```bash
$ mv image.png abc.txt
$ file abc.txt
abc.txt: PNG image data, 800 x 600, 8-bit/color RGB, non-interlaced
```



### 2.2 判断文件的编码格式

~~~sh
$ file README.md
README.md: GB2312 Unicode text, with CRLF line terminators
~~~

可以用`iconv` 工具来转换文档格式

~~~sh
$ iconv -f GB2312 -t UTF-8 README.md > README.md
~~~



### 2.3  查看可执行文件的架构和类型，是否strip（对开发者极有用）

```bash
$ file /bin/ls
/bin/ls: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=36b86f957a1be53733633d184c3a3354f3fc7b12, for GNU/Linux 3.2.0, stripped

$ file helloworld.elf
helloworld: ELF 64-bit LSB executable, UCB RISC-V, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-riscv64-lp64d.so.1, for GNU/Linux 4.15.0, with debug_info, not stripped
```

这个输出告诉你：
- 是 ELF 可执行文件
- 32/64 位
- x86-64 架构还是RISC-V 等
- 动态链接还是静态链接
- 使用的解释器
- 是否被 strip （去符号），`stripped` 表示符号表被移除，无法用 GDB 调试。







