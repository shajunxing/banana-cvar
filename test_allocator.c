#include "var.h"
#include <locale.h>

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
    printf("%lld\n", sizeof s);
}

void test_buffer() {
    // 结构体
    struct buffer buf1;
    bfinit(&buf1, sizeof(struct foo));
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
    bfpush(&buf1, &v4, 1);
    bfdump(&buf1);
    struct foo vout[20] = {0};
    bfpop(&buf1, vout, 5);
    cadumpall(vout);
    bfdump(&buf1);
    bfstrip(&buf1);
    bfdump(&buf1);
    // 字符串
    struct buffer buf2;
    bfinit(&buf2, sizeof(struct foo));
    bfpushall(&buf2, "Hello");
    bfpushall(&buf2, ", ");
    bfpushall(&buf2, "World!");
    bfdump(&buf2);
}

int main() {
    setlocale(LC_ALL, ".UTF-8");
    logdebug("==================== 测试stringbuffer ====================");
    const char *s = "lasdjlkasdjkajsdlkajdlkajsdklajdlkajdkajdjalsdjlasdjlkd";
    struct stringbuffer sb;
    sbinit(&sb);
    for (int i = 0; i < strlen(s); i++) {
        sbappend_s(&sb, s + i, 1);
    }
    puts(s);
    puts(sb.base);
    sbclear(&sb);
    logdebug("==================== 测试varbuffer ====================");
    struct var *vars[] = {nnew(0), nnew(1), nnew(2), nnew(3), nnew(4), nnew(5), nnew(6), nnew(7), nnew(8), nnew(9)};
    struct varbuffer vb;
    vbinit(&vb);
    for (int i = 0; i < countof(vars); i++) {
        vbpush(&vb, vars[i]);
    }
    vbdump(&vb);
    vbclear(&vb);
    vbdump(&vb);
    logdebug("==================== 测试objnodebuffer，rehash次数 ====================");
    odeclare(obj);
    oput(obj, "a", znew());
    oput(obj, "b", znew());
    oput(obj, "c", znew());
    oput(obj, "d", znew());
    oput(obj, "e", znew());
    oput(obj, "f", znew());
    oput(obj, "g", znew());
    oput(obj, "h", znew());
    oput(obj, "i", znew());
    oput(obj, "j", znew());
    oput(obj, "k", znew());
    oput(obj, "l", znew());
    oput(obj, "m", znew());
    oput(obj, "n", znew());
    oput(obj, "o", znew());
    oput(obj, "p", znew());
    oput(obj, "q", znew());
    oput(obj, "r", znew());
    oput(obj, "s", znew());
    oput(obj, "t", znew());
    oput(obj, "u", znew());
    oput(obj, "v", znew());
    oput(obj, "w", znew());
    oput(obj, "x", znew());
    oput(obj, "y", znew());
    oput(obj, "z", znew());
    logdebug("==================== 测试通用buffer ====================");
    test_buffer();
}