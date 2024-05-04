# A simplified, fast, transparent, and robust dynamic extension of C language, supports garbage collection, and is compatible with Json data types

English README | [中文自述文件](自述文件.md)

[Project address](https://github.com/shajunxing/banana-cvar)

## Updated May 4, 2024

After unremitting efforts, the conversion of numbers and strings to and from is realized manually, and the processing speed of values is now much faster than that of CJSON.

```
./test.exe
        ../nativejson-benchmark-1.0.0/data/twitter.json
                test_cjson_dec      : 0.078125
                test_cvar_dec       : 0.203125
                                    : 260%
                test_cjson_dec_enc  : 0.109375
                test_cvar_dec_enc   : 0.187500
                                    : 171%
        ../nativejson-benchmark-1.0.0/data/citm_catalog.json
                test_cjson_dec      : 0.140625
                test_cvar_dec       : 0.375000
                                    : 267%
                test_cjson_dec_enc  : 0.218750
                test_cvar_dec_enc   : 0.453125
                                    : 207%
        ../nativejson-benchmark-1.0.0/data/canada.json
                test_cjson_dec      : 0.656250
                test_cvar_dec       : 0.546875
                                    : 83%
                test_cjson_dec_enc  : 2.406250
                test_cvar_dec_enc   : 0.906250
                                    : 38%
./test.py
        ../nativejson-benchmark-1.0.0/data/twitter.json
                test_python_dec     : 0.062500
                test_python_dec_enc : 0.109375
        ../nativejson-benchmark-1.0.0/data/citm_catalog.json
                test_python_dec     : 0.125000
                test_python_dec_enc : 0.250000
        ../nativejson-benchmark-1.0.0/data/canada.json
                test_python_dec     : 0.562500
                test_python_dec_enc : 1.312500
```

## Updated May 3, 2024

Improved the codec of json, fixed a few bugs, and compared it with [cjson](https://github.com/DaveGamble/cJSON), I used a profiling sample taken from [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark). After continuous optimization, the current result is:


```
../nativejson-benchmark-1.0.0/data/twitter.json
    test_cjson_dec     : 0.078125
    test_cvar_dec      : 0.156250
                       : 200%
    test_cjson_dec_enc : 0.093750
    test_cvar_dec_enc  : 0.218750
                       : 233%
../nativejson-benchmark-1.0.0/data/citm_catalog.json
    test_cjson_dec     : 0.140625
    test_cvar_dec      : 0.390625
                       : 278%
    test_cjson_dec_enc : 0.203125
    test_cvar_dec_enc  : 0.625000
                       : 308%
../nativejson-benchmark-1.0.0/data/canada.json
    test_cjson_dec     : 0.656250
    test_cvar_dec      : 1.125000
                       : 171%
    test_cjson_dec_enc : 2.437500
    test_cvar_dec_enc  : 3.093750
                       : 127%
```

Among them, the citm_catalog.json mainly contains objects, canada.json are mostly arrays, twitter.json between the two, and the optimization is not much different from CJSON, CJSON does not deal with the key conflict of objects, it is just stored as array, In addition, the fastest [rapidjson](https://github.com/Tencent/rapidjson) and the fastest [yyjson](https://github.com/ibireme/yyjson) in the universe are even reused in the storage area of the source string, and my cvar is a full dynamic type system rather than just json functionality, and there is no comparison. The only thing that surprised me was that Python was as fast as CJSON, and Python is also a complete dynamic type system, I don't know what black magic is used, maybe copying strings and hashing algorithms is more efficient? Here are the results of Python.

```
../nativejson-benchmark-1.0.0/data/twitter.json
    test_python_dec     : 0.062500
    test_python_dec_enc : 0.109375
../nativejson-benchmark-1.0.0/data/citm_catalog.json
    test_python_dec     : 0.125000
    test_python_dec_enc : 0.234375
../nativejson-benchmark-1.0.0/data/canada.json
    test_python_dec     : 0.562500
    test_python_dec_enc : 1.312500
```

The following is the performance test code for C and Python, respectively.

```
#include <locale.h>
#include <windows.h>
#include <var.h>
#include "cJSON-1.7.17/cJSON.h"

char *s;
size_t slen;

void test_cjson_dec() {
    cJSON *jsonobj = cJSON_ParseWithLength(s, slen);
    cJSON_Delete(jsonobj);
}

void test_cjson_dec_enc() {
    cJSON *jsonobj = cJSON_ParseWithLength(s, slen);
    char *jsonstr = cJSON_PrintUnformatted(jsonobj);
    free(jsonstr);
    cJSON_Delete(jsonobj);
}

void test_cvar_dec() {
    vfromjson_s(s, slen);
    gc();
}

void test_cvar_dec_enc() {
    vtojson(vfromjson_s(s, slen));
    gc();
}

// https://stackoverflow.com/questions/1695288/getting-the-current-time-in-milliseconds-from-the-system-clock-in-windows
#define ft2ns(ft) (LONGLONG)(ft).dwLowDateTime + ((LONGLONG)((ft).dwHighDateTime) << 32LL)

double process_time() {
    FILETIME ct, et, kt, ut;
    if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
        return 0;
    }
    return (ft2ns(kt) + ft2ns(ut)) / 10000000.0;
}

double measure(void (*callback)(), size_t repeat) {
    double start = process_time();
    for (int i = 0; i < repeat; i++) {
        callback();
    }
    double end = process_time();
    return end - start;
}

void test_file(const char *filename) {
    printf("%s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error %d: %s\n", errno, strerror(errno));
        return;
    }
    struct stringbuffer jsonbuf;
    sbinit(&jsonbuf);
    char cache[1024];
    size_t numread;
    while ((numread = fread(cache, 1, sizeof cache, fp)) > 0) {
        sbappend_s(&jsonbuf, cache, numread);
    }
    fclose(fp);
    s = jsonbuf.base;
    slen = jsonbuf.length;
    size_t repeat = 10;
    double t1, t2;
    printf("    test_cjson_dec     : %lf\n", t1 = measure(test_cjson_dec, repeat));
    printf("    test_cvar_dec      : %lf\n", t2 = measure(test_cvar_dec, repeat));
    printf("                       : %.0lf%%\n", 100 * t2 / t1);
    printf("    test_cjson_dec_enc : %lf\n", t1 = measure(test_cjson_dec_enc, repeat));
    printf("    test_cvar_dec_enc  : %lf\n", t2 = measure(test_cvar_dec_enc, repeat));
    printf("                       : %.0lf%%\n", 100 * t2 / t1);
    sbclear(&jsonbuf);
    return;
}

int main() {
    setlocale(LC_ALL, ".UTF-8");
    test_file("../nativejson-benchmark-1.0.0/data/twitter.json");
    test_file("../nativejson-benchmark-1.0.0/data/citm_catalog.json");
    test_file("../nativejson-benchmark-1.0.0/data/canada.json");
}
```

```
#!/usr/bin/env python3

import json
import time

def test_python_dec(s):
    json.loads(s)

def test_python_dec_enc(s):
    jsonobj = json.loads(s)
    json.dumps(jsonobj)

def measure(cb, s):
    start = time.process_time()
    for i in range(10):
        cb(s)
    end = time.process_time()
    return end - start

def test_file(filename):
    print(filename)
    fp = open(filename, encoding="utf8")
    s = fp.read()
    fp.close()
    print("    test_python_dec     : %f" % measure(test_python_dec, s))
    print("    test_python_dec_enc : %f" % measure(test_python_dec_enc, s))

if __name__ == "__main__":
    test_file("../nativejson-benchmark-1.0.0/data/twitter.json")
    test_file("../nativejson-benchmark-1.0.0/data/citm_catalog.json")
    test_file("../nativejson-benchmark-1.0.0/data/canada.json")
    # test_file("../nativejson-benchmark-1.0.0/data/jsonchecker/pass01.json")
```

## Design ideas

Traditional collectors, such as Boehm, are all made on malloc/realloc/free, because C data structures are unpredictable, there is no special markup in memory, it is difficult to distinguish between pointers and data, such as Boehm GC can only do its best to judge. The high-level language defines a complete set of data structures by itself, prohibits users from directly manipulating memory, and the pointers and data are accurately recorded, which can be theoretically ensured, but it seems too cumbersome for the C language. I've always felt that C++/Rust routines are too ugly, because the low-level language niche is to completely expose the underlying data/memory structure, why make so many concepts? The bottom layer is completely opaque, and it is better to use a high-level language.

So my idea is to find the best compromise, follow the most accurate routine, make a fuss on the pointer, more than ten years of experience, Json types are enough, the root pointer of the marker removal algorithm is the variables created on the stack in the C language, record their state, and then correctly judge whether it is invalid, the leaf pointer is created on the heap, because the data structure is fixed, it can be easily recursively processed. And it's still essentially C code, the underlying structure is fully exposed, and it's just as performant as C, because it's simple to design, and it's easy to optimize and scale.

The corresponding C structure of dynamic variables is struct var, the type refers to the Json standard, which is null/boolean/number/array/object, and all dynamic variables are hosted on the globally linked list pvarroot. The operation of dynamic variables is in the form of pointers, that is, struct var *, and the pointers themselves are also variables, which are divided into two categories: one is stack variables, which are directly named in the source code, called references, and are also managed, using struct ref to record information and hang on the global linked list prefroot, and the other is heap variables, which are dynamically created and anonymous, such as pointers to child variables in array/object. The most important thing in the garbage collection algorithm is the principle of judging the invalidity of the reference, that is, the address that the reference now points to is different from the address pointed to by the previous registration, such as the stack value changed or was artificially modified after the function call was returned.

Developed in the GCC environment, using the latest C standard and GCC extension syntax, for exception handling, all unrecoverable errors are executed to exit the process, and the stack trace uses the libbacktrace library. I'm developing in a Windows environment, and the makefile is for MSYS2.

## Function naming conventions

Optional prefix + name + optional suffix, the meaning of the prefix is as follows.

|prefix|description|
|-|-|
|v|dynamically typed variables|
|r|reference|
|z|zero, zilch, zip, which represent null, nil, nought, nothing|
|b|boolean|
|n|number|
|s|string|
|a|array|
|o|object|
|w|world, whole, which means the whole and the whole |
|sb|stringbufferhelper function|

The suffix _s indicates a secure version of the function, such as a function containing an array of character parameters, if it is a string literal, it can be used as an unsafe function, and in other cases it is recommended to use a safe function, that is, the length of the string must be specified.

## Declarations and assignments of variables

If there are no special circumstances, be sure to use the provided macro to create variables, the function of the macro is to call the refer function to register the pointer variable, the unregistered is equivalent to a weak reference, which will be deleted when garbage collection; in addition, the macro will assign a value to the reference, which is necessary, because most functions do not allow the NULL value as a parameter, which will trigger an abnormal exit; assign a variable to b variable, the actual pass is the address; all assigned number/ The original value of string is to create a new variable instead of modifying the original variable, and array/object has a set of functions to add, delete and modify.

The following table lists functions and macros, both with arguments and return value types are functions, and vice versa are macros. The so-called dynamic syntax is to give an intuitive feeling with a JavaScript-like syntax, and in the future, it may be extended in this way and a converter can be written to convert it to C code, so that it can perfectly replace embedded dynamic languages such as Lua.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|vdeclare(a, b)|var a = b|declare macros are assigned to declare variables at the same time
|zdeclare(a)|var a = null<br>, i.e. var a|null|
|bdeclare(a, b)|var a = true|, assign a boolean value <br>bdeclare(a, true)|
|ndeclare(a, b)|var a = 3.14|assign the value <br>ndeclare(a, 3.14)|
|sdeclare(a, b)|var a = "hello"|assign the string <br>sdeclare(a, "hello")|
|adeclare(a)|var a = []|null array<br>|
|odeclare(a)|var a = {}|nullify object<br>|
|vassign(a, b)|a = b|assign a new value to an existing variable
|zassign(a)|a = null|null|null|null|
|bassign(a, b)|a = true|, assign the boolean value <br>bassign(a, true)|
|nassign(a, b)|a = 3.14|<br>nassign(a, 3.14)|
|sassign(a, b)|a = "hello"|assign the string <br>sassign(a, "hello")|
|aassign(a)|a = []|nullify array<br>|
|oassign(a)|a = {}|nullify object<br>|
|struct var *znew()||Create a null value. The new series of functions to create new dynamic variables, note that this kind of function is generally not used directly, but is included in the declare and assign series of macros, in order to save memory, null/true/false are all global constants|
|struct var *bnew(bool b)||Create a Boolean value|
|struct var *nnew(double n)||Create a numeric value|
|struct var *snew_s(const char *s, size_t slen)<br>var *snew(const char *sz)||Create a string|
|struct var *anew(size_t num, ...)|var a = [null, true, 3.14, "hello"]|Create an array of 0 or more <br>membersvdeclare(a, anew(4, znew(), bnew(true), nnew(3.14), snew("hello")))|
|struct var *onew()||Create an empty object|

## Generic and helper functions

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|lambda(ret, args, body)||Use GCC's compound statements and nested function syntax to define the anonymous function <br>lambda(int, (int a, int b), {return a + b;})|
|void stacktrace()||Print stack trace information|
|void alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz)||Secure Request Memory|
|void free_s(void **pp)||Securely free memory|
|void sbinit(struct stringbuffer *psb)||Initialize the stringbuffer, the initial capacity is defined by the macro buffer_initial_capacity_exponent, note that the value is an exponent of a power of 2|
|void sbclear(struct stringbuffer *psb)||After the stringbuffer is used, you need to call this function to release memory |
|void sbappend_s(struct stringbuffer *psb, const char *s, size_t slen)<br>void sbappend(struct stringbuffer *psb, const char *sz)||Append the specified string to the end of the |
|void sbdump(struct stringbuffer *psb)||Print the debug information of the stringbuffer|
|void vdump(const char *prefix, const struct var *pv)||Print debug information for a single dynamic variable|
|void wdump(const char *suffix)<br>dump()||Prints all constants, variables, and references|
|void gc()||Garbage Collection|
|void refer(struct var **ppv, char *descr, struct var *pval)||To register or update reference information, this function is generally not called directly, but is included in the declare and assign series of macros
|enum vtype vtype(struct var *pv)||Returns the variable type in the enum format, with the following values: vtnull, vtboolean, vtnumber, vtstring, vtarray, and vtobject|

## Boolean

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|bool bvalue(struct var *pv)||Returns the original value of the boolean variable|

## Numeric

Values are numbers, and they do not distinguish between integers and floating-point numbers, and can be replaced with other distinguishable or high-precision numerical computing libraries if necessary, but do not distinguish at the API level.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|double nvalue(struct var *pv)||Returns the original value of the number variable|

## Strings

All of the internal buffer storage structures I've designed, including strings, arrays, and objects, are uniform and optimized, but I haven't thought of a more appropriate API interface for string connections just yet.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|const char *svalue(struct var *pv)||Returns the original value of the string variable (pointing to an array of characters ending in 0)|
|size_t slength(struct var *pv)|a.length()|returns string length|
|struct var *sconcat(size_t num, ...)|var a = "hi" <br>var b = "all" <br>var c = a + b|concatenate one or more <br>strings sdeclare(a, "hi")<br>sdeclare(b, "all")<br>vdeclare(c, sconcat(2, a, b))|
|struct var *sformat(const char *fmt, ...)||Return the formatted string, with the same parameters as printf|

## Arrays

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|void aclear(struct var *pv)|v.clear()|empty array|
|size_t alength(struct var *pv)|v.length()|returns array length|
|void apush(struct var *pv, struct var *pval)|v.push(val)|insertion at the end of the array, length +1|
|struct var *apop(struct var *pv)|v.pop()|array pops up at the end of the array, length -1|
|void aput(struct var *pv, size_t idx, struct var *pval)|v[idx] = val|array specifies that the subscript is written to replace the original value, and the program exits | if the subscript is out of bounds
|struct var *aget(struct var *pv, size_t idx)|v[idx]|takes the value of the array to specify the subscript, and the program exits if the subscript is out of bounds|
|void asort(struct var *pv, int (*comp)(const struct var *, const struct var *))|v.sort()|array sorting, comp is a user-specified sorting function, the return value convention is the same as qsort, if it is NULL, then call the default sorting function, the rules are: 1. Different types, according to null, boolean, number, 2. The string is called memcmp with the minimum length +1 before true, that is, 0 at the end participates in the comparison, and the array and object only compare the number of elements |
|void aforeach(struct var *arr, void (*cb)(size_t i, struct var *v))|for (i, v in arr) {...}|Sequentially iterate through all indexes and values of the array|

## Objects

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|void oclear(struct var *pv)|v.clear()|empty object|
|size_t olength(struct var *pv)|v.length()|returns the number of elements of the object|
|void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval)<br>void oput(struct var *pv, const char *key, struct var *pval)|v[key] = val|write key-value pair|
|struct var *oget_s(struct var *pv, const char *key, size_t klen)<br>struct var *oget(struct var *pv, const char *key)|v[key]|read the value corresponding to the key, note that NULL may be returned, and the program must make a judgment|
|void oforeach(struct var *obj, void (*cb)(const char *k, size_t klen, struct var *v))|for (k, klen, v in obj) {...}|Iterate through all keys and values of the object|

## Json

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|struct var *vtojson(struct var *pv)||jsonize arbitrary variables, and the result is returned as a string variable, note: nesting dolls are prohibited, and nested loops are ignored|
|tojson(pv)|print([null, [3.140000, "hi"], false])|Returns an array of C characters jsonized by the variable, which is used for functions such as printf<br>printf(tojson(anew(3, znew(), anew(2, nnew(3.14), snew("hi")), bnew(false))))|
|struct var *vfromjson_s(const char *jsonstr, size_t jsonslen)<br>struct var *vfromjson(const char *jsonstr)||Parsing JSON strings, this function will skip all non-essential characters, so it has a strong error correction ability, in addition, UTF-16 escape is not supported, because it has long been abandoned, JSON strings themselves are forced UTF-8 encoding|

## Code sample

For more sample code, see test_var.c




