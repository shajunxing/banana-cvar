#include "var.h"

#define lambda(ret, args, body) ({ ret f args body &f; })

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

void foo(void (*fn)()) {
    fn();
    stacktrace();
}

int main() {
    // int i = 1;
    // foo(lambda(void, (), { i++; stacktrace(); }));
    // stacktrace();
    // printf("%d\n", i);
    // errno = 0;
    // printf("%d %s\n", errno, strerror(errno));
    // errno_t e;
    // exitif(NULL == 0);
    // exitif(NULL == 0, E2BIG);
    alloc_s(NULL, 10, 0, 1);
    // printf("%d\n", is_empty());
    // printf("%d\n", is_empty(foo, bar, baz));
    vdeclare(a, NULL);
    return 0;
}
