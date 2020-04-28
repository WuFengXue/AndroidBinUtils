
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