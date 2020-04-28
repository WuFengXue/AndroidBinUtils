
# 说明 / intro

* 该工程下的工具主要运行在安卓手机上，请将工具用 adb push 到手机后使用

* These bin utils are work only on android devices, please push to your devices first (use adb push cmd)

* 发布的版本只提供了 armeabi-v7a 架构的版本，如果需要其他版本，请自行编译

* Release versions only contain armeabi-v7a, if you need other arch, please compile by yourself

# elftag

* modify app_process32(64) to load third so lib

## reference

* [一种简单的Android全局注入方案](https://bbs.pediy.com/thread-224191.htm)

* [修改android app_process elf (实现rrrfff大神 <android全局注入>第一步)](https://bbs.pediy.com/thread-224297.htm)

## usage

```
usage: elftag [option] <elffile>
 modify dynamic section tag of DEBUG to NEEDED and set its value to android_runtime.so
 Options are:
  -r        Revert modification to DEBUG tag
  -h        Display this information
```

# FixElfSection

* 用于dump elf文件后的section修复，修复后可以在IDA中直接查看

* [FixElfSection](https://github.com/WangYinuo/FixElfSection) 的 AndroidStudio 实现

## reference

* [ELF文件格式学习，section修复](https://blog.csdn.net/yi_nuo_wang/article/details/72626846)

* [Android so库文件的区节section修复代码分析](https://blog.csdn.net/qq1084283172/article/details/78818917)

## usage

```shell
usage: FixElfSection <elffile>
 fix.so will be created in the same directory
```

# SoFixer

* so修复相关

* [SoFixer](https://github.com/F8LEFT/SoFixer) 的 AndroidStudio 实现

## reference

* [简单粗暴的so加解密实现](https://bbs.pediy.com/thread-191649.htm)

* [ELF section修复的一些思考](https://bbs.pediy.com/thread-192874.htm=%3E)

## usage

```shell
SoFixer v0.2 author F8LEFT(currwin)
Useage: SoFixer <option(s)> -s sourcefile -o generatefile
 try rebuild shdr with phdr
 Options are:
  -d --debug                                 Show debug info
  -m --memso memBaseAddr(16bit format)       Source file is dump from memory from address x
  -s --source sourceFilePath                 Source file path
  -o --output generateFilePath               Generate file path
  -h --help                                  Display this information
```