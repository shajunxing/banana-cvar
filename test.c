#include "var.h"

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


#pragma pack(push, 1)
struct foo {
    int a;
    char b;
};
#pragma pack(pop)

void cadump(void *base, size_t length, size_t size) {
    printf("[ ");
    char *offset = (char *)base;
    for (int i = 0; i < length; i++) {
        if (i > 0) {
            printf(" | ");
        }
        for (int j = 0; j < size; j++) {
            if (j > 0) {
                printf(",");
            }
            printf("%02X", *(offset + j));
        }
        offset += size;
    }
    printf(" ]\n");
}

#define cadumpall(arr) cadump(arr, countof(arr), sizeof((arr)[0]))

void bar(size_t i) {
    char s[i];
    printf("%d\n", sizeof s);
}

int main() {
    // 结构体
    struct buffer buf1 = bufferof(struct foo);
    struct foo v1[] = {{1, 'a'}, {2, 'b'}, {3, 'c'}};
    struct foo v2[] = {{4, 'd'}, {5, 'e'}, {6, 'f'}, {7, 'g'}};
    struct foo v3[] = {{8, 'h'}, {9, 'i'}};
    cadumpall(v1);
    cadumpall(v2);
    cadumpall(v3);
    bfpushall(&buf1, v1);
    bfpushall(&buf1, v2);
    bfpushall(&buf1, v3);
    struct foo v4 = {10, 'j'};
    bfpushall(&buf1, &v4);
    bfdump(&buf1);
    struct foo vout[20] = {0};
    bfpop(&buf1, vout, 5);
    cadumpall(vout);
    bfdump(&buf1);
    bfstrip(&buf1);
    bfdump(&buf1);
    //字符串
    struct buffer buf2 = bufferof(char);
    bfpushall(&buf2, "Hello");
    bfpushall(&buf2, ", ");
    bfpushall(&buf2, "World!");
    bfdump(&buf2);
    return 0;
}