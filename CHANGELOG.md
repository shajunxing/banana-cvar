# A simplified, fast, transparent, and robust dynamic extension of C language, supports garbage collection, and is compatible with Json data types

[English Readme](README.md) | [中文自述文件](自述文件.md) | [English Changelog](CHANGELOG.md) | [中文更改日志](更改日志.md)

[Project address](https://github.com/shajunxing/banana-cvar)

## Updated May 11, 2024

Added string pattern matching.

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
    sbfree(&jsonbuf);
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

