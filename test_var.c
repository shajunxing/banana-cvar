#include "var.h"
#include <locale.h>

struct var *test1() {
    vdeclare(a, snew("Hello"));
    dump();
    return a;
}

// 数组测试
void test_array() {
    vdeclare(a, anew(3, bnew(true), nnew(3.14), snew("Fuck You All")));
    dump();
    aput(a, 2, snew("操你妈"));
    dump();
    printf("%s %s %s\n", tojson(apop(a)), tojson(apop(a)), tojson(aget(a, 0)));
    dump();
    vdeclare(b, anew(2, bnew(false), a));
    apush(a, b);
    puts(tojson(a));
    dump();
    aclear(a);
    dump();
}

// 数组内存泄漏测试
void test_array_memleak() {
    vdeclare(a, anew(0));
    vdeclare(b, bnew(true));
    vdeclare(c, nnew(3.14));
    vdeclare(d, snew("Fuck You All"));
    for (;;) {
        apush(a, b);
        apush(a, c);
        apush(a, d);
        apop(a);
        apop(a);
        aclear(a);
    }
}

void test_string() {
    sconcat(3, snew("你好"), snew("丢你"), snew("老木"));
    dump();
}

void test_json() {
    vdeclare(a, anew(0));
    printf("%s\n", svalue(vtojson(a)));
    dump();
    apush(a, znew());
    apush(a, bnew(true));
    apush(a, bnew(false));
    apush(a, nnew(3.141592654));
    apush(a, snew("你好世界"));
    printf("%s\n", svalue(vtojson(a)));
    vdeclare(b, anew(0));
    apush(b, nnew(2.718281829));
    apush(b, snew("HelloWorld"));
    apush(a, b);
    printf("%s\n", svalue(vtojson(a)));
    dump();
}

void test_object() {
    vdeclare(a, onew());
    oput(a, "foo", znew());
    dump();
    oput(a, "bar", bnew(true));
    dump();
    oput(a, "baz", nnew(3.14));
    dump();
    oput(a, "abc", snew("wow"));
    dump();
    oput(a, "abc", snew("haha"));
    dump();
    printf("%p\n", oget(a, "foo"));
    printf("%p\n", oget(a, "bar"));
    printf("%p\n", oget(a, "baz"));
    printf("%p\n", oget(a, "abc"));
    printf("%p\n", oget(a, "xyz"));
    printf("%s\n", tojson(a));
    vdeclare(b, anew(4, bnew(false), a, nnew(2.718), snew("特别军事行动")));
    oput(a, "arrrrrrrrrr", b);
    printf("%s\n", tojson(a));
    printf("\n");
    dump();
    oclear(a);
    aclear(b);
    dump();
}

void test_object_memleak() {
    odeclare(a);
    zdeclare(b);
    bdeclare(c, true);
    ndeclare(d, 3.14);
    sdeclare(e, "hahaha");
    adeclare(f);
    for (;;) {
        oput(a, "foo", b);
        oput(a, "bar", c);
        oput(a, "baz", d);
        oput(a, "abc", e);
        oput(a, "xyz", f);
        oclear(a);
    }
}

void test_gc() {
    for (;;) {
        zdeclare(a);
        bdeclare(b, true);
        ndeclare(c, 3.14);
        sdeclare(d, "Hello");
        vdeclare(e, anew(3, b, c, d));
        odeclare(f);
        oput(f, "foo", b);
        oput(f, "bar", c);
        oput(f, "baz", d);
        oput(f, "baz", e);
        apush(e, f);
        printf("%s\n%s\n\n", tojson(e), tojson(f));
        a = NULL;
        b = NULL;
        c = NULL;
        d = NULL;
        // e = NULL;
        // f = NULL;
        gc();
        dump();
    }
}

int main() {
    setlocale(LC_ALL, ".UTF-8");
    // vdeclare(b, test1());
    // dump(); // 可以看到函数退出后原先栈内的变量值变化了
    // vdeclare(a, nnew(10));
    // vassign(b, nnew(20));
    // dump();
    // vassign(a, nnew(nvalue(a) + nvalue(b)));
    // dump();
    // vassign(a, NULL);
    printf(tojson(anew(3, znew(), anew(2, nnew(3.14), snew("hi")), bnew(false))));
    return 0;
}
