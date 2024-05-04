#include "var.h"
#include <locale.h>

// bool buffered_reader(const char *fname, const char *mode, size_t bufsz, ssize_t (*cb)(const char *, size_t)) {
//     FILE *fp = fopen(fname, mode);
//     if (!fp) {
//         return false;
//     }
//     uint8_t buffer[bufsz] = {0};
//     size_t nbytes_read = 0;
//     ssize_t nbytes_fed = 0;
//     for (;;) {
//         if (nbytes_fed < nbytes_read) {
//             nbytes_fed = cb(buffer, nbytes_read);
//             if (nbytes_fed < 0) {
//                 return false;
//             }
//         } else {
//             nbytes_fed = 0;
//             nbytes_read = fread(buffer, 1, sizeof buffer, fp);
//             if (0 == nbytes_read) {
//                 break;
//             }
//         }
//     }
//     fclose(fp);
//     return true;
// }

// void foo(void (*fn)()) {
//     fn();
//     stacktrace();
// }

// int main() {
// int i = 1;
// foo(lambda(void, (), { i++; stacktrace(); }));
// stacktrace();
// printf("%d\n", i);
// errno = 0;
// printf("%d %s\n", errno, strerror(errno));
// errno_t e;
// exitif(NULL == 0);
// exitif(NULL == 0, E2BIG);
// alloc_s(NULL, 10, 0, 1);
// printf("%d\n", is_empty());
// printf("%d\n", is_empty(foo, bar, baz));
//     vdeclare(a, NULL);
//     return 0;
// }

// void test_jsonfile(const char *filename) {
//     puts(filename);
//     FILE *fp = fopen(filename, "r");
//     if (fp == NULL) {
//         printf("Error %d: %s\n", errno, strerror(errno));
//         return;
//     }
//     struct stringbuffer jsonbuf;
//     sbinit(&jsonbuf);
//     char cache[1024];
//     size_t numread;
//     while ((numread = fread(cache, 1, sizeof cache, fp)) > 0) {
//         sbappend_s(&jsonbuf, cache, numread);
//     }
//     fclose(fp);
//     puts(jsonbuf.base);
//     vdeclare(jsonobj, vfromjson_s(jsonbuf.base, jsonbuf.length));
//     puts(tojson(jsonobj));
//     sbclear(&jsonbuf);
// }

// double atof_s(const char *s, size_t slen) {
//     char *base = (char *)s;
//     size_t offset = 0;
//     bool
//     long inte = 0, frac = 0, expo = 0;
//     for (;offset < slen;offset++) {
//         char ch = *(base + offset);
//     }
// }

int main() {
    setlocale(LC_ALL, ".UTF-8");
    // for (int j = 0; j <= 33; j++) {
    //     test_jsonfile(svalue(sformat("../nativejson-benchmark-1.0.0/data/jsonchecker/fail%02d.json", j)));
    // }
    // test_jsonfile("../nativejson-benchmark-1.0.0/data/jsonchecker/pass01.json");
    // gc();
    double num = -1234.56789;
    if (num < 0) {
        putchar('-');
        num = -num;
    }
    int ilen = 0;
    for (; num >= 1; num /= 10) {
        ilen++;
    }
    if (ilen == 0) {
        putchar('0');
    } else {
        for (int i = 0; i < ilen; i++) {
            num *= 10;
            int inte = (int)num;
            putchar(inte + '0');
            num -= inte;
        }
    }
    putchar('.');
    for (; num > 0.01;) {
        num *= 10;
        int inte = (int)num;
        putchar(inte + '0');
        num -= inte;
    }
    putchar('\n');
    return 0;
}