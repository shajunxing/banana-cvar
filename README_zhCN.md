# Banana CVar，一套支持垃圾回收的C语言高级数据类型

本文使用 [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) 许可。

[英文版](README.md) | [中文版](README_zhCN.md)

项目地址：<https://github.com/shajunxing/banana-cvar>

这是一个在C程序中使用项目 <https://github.com/shajunxing/banana-script> 中的数据类型和垃圾回收器的范例。C程序的垃圾回收判断准则很简单：记录每一个变量的值和地址，如果地址指向的值和记录的值一致，则标记为正在使用中，因为一致意味着调用栈没有变化；反之如果离开了该变量所在的作用域，比如函数退出，这两者迟早会不一致的。

如何构建参见 <https://github.com/shajunxing/banana-script>，不再赘述。

在`example.c`中，把`gc();`这行注释掉，在任务管理器里观察，内存就会不断增长，因为`var f = string("This is a dynamic string");`是动态分配内存的，拿掉注释，内存则稳定不增长，说明垃圾回收策略非常成功。

```c
#include <stdlib.h>
#include "var.h"

void foo() {
    var(a, null());
    var(b, boolean(true));
    var(c, boolean(false));
    var(d, number(3.14));
    var(e, scripture("This is a constant string"));
    var(f, string("This is a dynamic string"));
    print(a, b, c, d, e, f);
}

int main() {
    for(;;) {
        foo();
        gc();
    }
    return EXIT_SUCCESS;
}
```