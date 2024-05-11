#include "var.h"

void array_callback(size_t i, struct var *v, void *xargs) {
    printf("%lld: %s\n", i, tojson(v));
}

void object_callback(const char *k, size_t klen, struct var *v, void *xargs) {
    printf("%s: %s\n", k, tojson(v));
}

void call() {
    odeclare(obj);
    oput(obj, "alpha", bnew(true));
    oput(obj, "beta", nnew(3.14));
    vdeclare(arr, anew(3, snew("hello"), snew("world"), nnew(2.718)));
    puts("\e[91m");
    aforeach(arr, array_callback, NULL);
    oput(obj, "gamma", arr);
    oput(obj, "delta", vfromjson("{\"object with 1 member\":[\"array with 1 element\"]}"));
    oput(obj, "epsilon", sformat("%s %s %s %s", "this", "is", "from", "sformat"));
    puts("\e[92m");
    oforeach(obj, object_callback, NULL);
    puts("\e[93m");
    printf("%s\n", tojson(obj));
    puts("\e[94m");
    dump();
}

int main() {
    logdebug("This is a debug message");
    logerror("This is an error message and stack trace");
    stacktrace();
    ndeclare(a, 2.718);
    sdeclare(b, "greetings");
    call();
    puts("\e[95m");
    printf("After some following function calls, call stack will change,\n");
    printf("and all references and corresponding variables will be cleaned during gc:\n");
    gc();
    dump();
    puts("\e[0m");
    printf("Pattern matching example: %s\n", tojson(smatch("(([^=,%s]+)=([^=,%s]+))", "name=tom, age=33, gender=m")));
    return 0;
}
