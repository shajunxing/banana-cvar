#include "var.h"

void call1() {
    odeclare(obj);
    oput(obj, "first", bnew(true));
    oput(obj, "second", nnew(3.14));
    oput(obj, "third", anew(3, snew("hello"), snew("world"), nnew(2.718)));
    dump();
    printf("%s\n\n", tojson(obj));
}

// void call2() {
//     char foo[100];
//     memset(foo, 0xff, sizeof foo);
// }

int main() {
    call1();
    // call2(); // call stack will be changed
    printf("Hello\n"); // call stack will be changed
    gc(); // and all references inside call1() scope will be invalidated
    dump();
    return 0;
}
