#include "var.h"

void array_callback(size_t idx, struct var *val) {
    printf("%lld: %s\n", idx, tojson(val));
}

void object_callback(const char *key, size_t klen, struct var *val) {
    printf("%s: %s\n", key, tojson(val));
}

void call() {
    odeclare(obj);
    oput(obj, "alpha", bnew(true));
    oput(obj, "beta", nnew(3.14));
    vdeclare(arr, anew(3, snew("hello"), snew("world"), nnew(2.718)));
    aforeach(arr, array_callback);
    oput(obj, "gamma", arr);
    oput(obj, "delta", vfromjson("{\"object with 1 member\":[\"array with 1 element\"]}"));
    oput(obj, "epsilon", sformat("%s %s %s %s", "this", "is", "from", "sformat"));
    oforeach(obj, object_callback);
    printf("%s\n", tojson(obj));
    dump();
}

int main() {
    logdebug("This is a debug message");
    logerror("This is an error message and stack trace");
    stacktrace();
    ndeclare(a, 2.718);
    sdeclare(b, "greetings");
    call();
    printf("After some following function calls, call stack will change,\n");
    printf("and all references and corresponding variables will be cleaned during gc:\n");
    gc();
    dump();
    printf("Pattern matching example: %s\n", tojson(smatch("(([^=,%s]+)=([^=,%s]+))", "name=tom, age=33, gender=m")));
    return 0;
}
