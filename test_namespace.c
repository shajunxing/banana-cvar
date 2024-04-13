#include <stdio.h>

void __foo() {
    puts("foo");
}

void __bar() {
    puts("bar");
}

typedef void (*_foo)();
typedef void (*_bar)();

typedef struct {
    struct {
        _foo foo;
        _bar bar;
    } ns1;
} _ns2;

const _ns2 ns2 = {{__foo, __bar}};

int main(int argc, char *argv[]) {
    ns2.ns1.foo();
    ns2.ns1.bar();
    return 0;
}