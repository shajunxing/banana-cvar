# Banana CVar, a set of advanced data types in C that supports garbage collection

This article is openly licensed via [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).

[English Version](README.md) | [Chinese Version](README_zhCN.md)

Project Address: <https://github.com/shajunxing/banana-cvar>

This is an example of using data types and garbage collector from project <https://github.com/shajunxing/banana-script> in C program. Rule for garbage collection in C program is quite simple: Record value and address of each variable. If value at address matches recorded value, it's marked as being in use because that means call stack hasn't changed. On the other side, if leave that variable scope, such as when function exits, these two will eventually not match.

How to build see <https://github.com/shajunxing/banana-script> and will not be repeated.

In `example.c`, if you comment out line `gc();` and check task manager, you’ll see memory keeps growing. This happens because `var f = string("This is a dynamic string");` is dynamically allocating memory. But if you uncomment that line, memory stabilizes and doesn't grow, which shows that garbage collection strategy is really successful.

```c
#include <stdlib.h>
#include "var.h"

void foo() {
    var a = null();
    var b = boolean(true);
    var c = boolean(false);
    var d = number(3.14);
    var e = scripture("This is a constant string");
    var f = string("This is a dynamic string");
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