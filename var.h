/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef VAR_H
#define VAR_H

// 比如vasprintf就需要声明_GNU_SOURCE
#define _GNU_SOURCE
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 参考 https://gcc.gnu.org/wiki/Visibility
// https://stackoverflow.com/questions/2810118/how-to-tell-the-mingw-linker-not-to-export-all-symbols
// https://www.linux.org/docs/man1/ld.html
// windows需要加上参数-Wl,--exclude-all-symbols，而linux用的是-fvisibility=hidden和-fvisibility-inlines-hidden
#ifdef DLL
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef EXPORT
            #define shared __declspec(dllexport)
        #else
            #define shared __declspec(dllimport)
        #endif
    #elif __GNUC__ >= 4
        #define shared __attribute__((visibility("default")))
    #else
        #define shared
    #endif
#else
    #define shared
#endif

#define str(a) #a
#define xstr(a) str(a)
#define countof(a) (sizeof(a) / sizeof((a)[0]))
#define concat(a, b) a##b
#define xconcat(a, b) concat(a, b)
// 注意lambda函数名不要取诸如f之类的，会重名
#define lambda(ret, args, body) ({ ret xconcat(__lambda_, __LINE__) args body &xconcat(__lambda_, __LINE__); })

#ifndef max
    #define max(a, b) \
        ({ __auto_type _a = (a); \
       __auto_type _b = (b); \
     _a > _b ? _a : _b; })
#endif
#ifndef min
    #define min(a, b) \
        ({ __auto_type _a = (a); \
       __auto_type _b = (b); \
     _a < _b ? _a : _b; })
#endif

// NDEBUG是习惯，比如assert宏
#ifndef NDEBUG
    #define logdebug(fmt, ...) \
        fprintf(stdout, "\e[38;2;0;102;255mDEBUG %s,%d %s:\e[0m " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
    #define logdebug(fmt, ...) \
        do {                   \
        } while (0)
#endif

#define logerror(fmt, ...) \
    fprintf(stderr, "\e[38;2;255;51;51mERROR %s,%d %s:\e[0m " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifndef NDEBUG
shared void stacktrace();
#else
    #define stacktrace() \
        do {             \
        } while (0)
#endif

shared void hexdump(void *start, size_t length);

#define logexit(fmt, ...)             \
    do {                              \
        logerror(fmt, ##__VA_ARGS__); \
        stacktrace();                 \
        exit(EXIT_FAILURE);           \
    } while (0)

// 如果cond为真则退出程序，可选参数为errno，如无则保留当前值不变
#define exitif(cond)        \
    do {                    \
        if (cond) {         \
            logexit(#cond); \
        }                   \
    } while (0)

shared void free_s(void **pp);
shared void alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz);

// 为了优化性能，设置buffer的初始容量，建议为2的幂，因为容量值的增长都是右移一位
// 经测试容量8比较合适，太大反而速度降低了
#define BUFFER_INITIAL_CAPACITY 8

// 通用buffer类型，计划未来替换下面的专用buffer
// 但是有一个问题，就是buffer里面记录size，与初始化时侯全部memset为0有点不协调

// 不设置destructor，因为push/pop不好弄
struct buffer {
    size_t size; // 每单元大小
    void *base;
    size_t capacity;
    size_t length;
};
shared void bfinit(struct buffer *pbuffer, size_t size);
shared void *bfoffset(struct buffer *pbuffer, size_t index);
shared void bffree(struct buffer *pbuffer);
shared void bfstrip(struct buffer *pbuffer);
shared void bfpush(struct buffer *pbuffer, void *pvalues, size_t nvalues);
#define bfpushall(pb, pv) bfpush(pb, pv, countof(pv))
shared void bfpop(struct buffer *pbuffer, void *pvalues, size_t nvalues);
#define bfpopall(pb, pv) bfpop(pb, pv, (pb)->length)
shared void bfput(struct buffer *pbuffer, size_t index, void *pvalue);
shared void bfget(struct buffer *pbuffer, size_t index, void *pvalue);
shared void bfdump(struct buffer *pbuffer);

// 字符串buffer类型，与通用buffer类型有区别
struct stringbuffer {
    char *base;
    size_t capacity;
    size_t length;
};
shared void sbinit(struct stringbuffer *psb);
shared void sbclear(struct stringbuffer *psb);
shared void sbfree(struct stringbuffer *psb);
shared void sbappend_s(struct stringbuffer *psb, const char *s, size_t slen);
shared void sbappend(struct stringbuffer *psb, const char *sz);
shared void sbdump(struct stringbuffer *psb);

enum vtype {
    vtnull,
    vtboolean,
    vtnumber,
    vtstring,
    vtarray,
    vtobject
};

struct var;

struct varbuffer {
    struct var **base;
    size_t capacity;
    size_t length;
};
shared void vbinit(struct varbuffer *pb);
shared void vbfree(struct varbuffer *pb);
shared void vbpush(struct varbuffer *pb, struct var *v);
shared struct var *vbpop(struct varbuffer *pb);
shared void vbput(struct varbuffer *pb, struct var *v, size_t i);
shared struct var *vbget(struct varbuffer *pb, size_t i);
shared ssize_t vbfind(struct varbuffer *pb, struct var *v);
shared void vbdump(struct varbuffer *pb);

struct objnode {
    struct stringbuffer key;
    struct var *value;
};

struct objnodebuffer {
    struct objnode *base;
    size_t capacity;
    size_t length;
};

struct var {
    bool inuse;
    enum vtype type;
    // null、boolean、number直接存储，其余存储指针
    union {
        bool bvalue;
        // 不要用long double，mingw有问题
        // 参见 https://stackoverflow.com/questions/7134547/gcc-printf-and-long-double-leads-to-wrong-output-c-type-conversion-messes-u
        double nvalue;
        struct stringbuffer svalue;
        struct varbuffer avalue;
        struct objnodebuffer ovalue;
    };
    struct var *prev;
    struct var *next;
    // 不能用把值附加在header末尾的方式，因为array/object的realloc会导致指针值变化
};

struct ref {
    char *descr; // 描述文本，调试显示用，总是指向栈值，无需释放
    // 变量指针的地址，注意变量指针必须始终指向实际存在的变量，因为变量是链表存放的，判断是否野指针需要遍历整个链表，代价太大，而变量如果数组存放，则又存在gc收缩时候修改指针地址的麻烦，因为不止具名指针，array/object的匿名指针都需要改
    struct var **addr; // 指针变量自身的地址
    struct var *value; // 记录该指针的记录时候的值，如果不等于当前实际的值，那么原因只可能是系统调用栈变化了，被系统回收了，则判定为野指针
    struct ref *prev;
    struct ref *next;
};

shared void vdump(const char *prefix, const struct var *pv, const char *suffix);
shared void rdump(const char *prefix, const struct ref *pr, const char *suffix);
shared void wdump();
#define dump()        \
    do {              \
        logdebug(""); \
        wdump();      \
    } while (0)

shared void gc();

shared void refer(struct var **ppv, char *descr, struct var *pval);
// a = b
// 注意GCC里的__func__ __FUNCTION__ __PRETTY_FUNCTION__是变量名不是字面量，无法用在宏里面字符串连接
// 注意side effect，b如果是表达式避免执行两次
#define vassign(a, b)                                            \
    do {                                                         \
        struct var *_b = (b);                                    \
        exitif(_b == NULL);                                      \
        refer(&(a), __FILE__ "," xstr(__LINE__) "," str(a), _b); \
    } while (0)
// var a = b
#define vdeclare(a, b)    \
    struct var *a = NULL; \
    vassign(a, b)

shared struct var *vnew();
shared enum vtype vtype(struct var *pv);

shared struct var *znew();
#define zdeclare(a) vdeclare(a, znew())
#define zassign(a) vassign(a, znew())

shared struct var *bnew(bool b);
#define bdeclare(a, b) vdeclare(a, bnew(b))
#define bassign(a, b) vassign(a, bnew(b))
shared bool bvalue(struct var *pv);

shared struct var *nnew(double n);
#define ndeclare(a, b) vdeclare(a, nnew(b))
#define nassign(a, b) vassign(a, nnew(b))
shared double nvalue(struct var *pv);

shared struct var *snew_s(const char *s, size_t slen);
shared struct var *snew(const char *sz);
#define sdeclare(a, b) vdeclare(a, snew(b))
#define sassign(a, b) vassign(a, snew(b))
shared const char *svalue(struct var *pv);
shared size_t slength(struct var *pv);
shared struct var *sconcat(size_t num, ...);
shared struct var *sformat(const char *fmt, ...);
shared struct var *smatch_s(const char *pat, const char *str, size_t slen);
shared struct var *smatch(const char *pat, const char *sz);

shared struct var *anew(size_t len, ...);
#define adeclare(a) vdeclare(a, anew(0))
#define aassign(a) vassign(a, anew(0))
shared void aclear(struct var *arr);
shared void afree(struct var *arr);
shared size_t alength(struct var *arr);
shared void apush(struct var *arr, struct var *val);
shared struct var *apop(struct var *arr);
shared void aput(struct var *arr, size_t idx, struct var *val);
shared struct var *aget(struct var *arr, size_t idx);
shared void asort(struct var *arr, int (*comp)(const struct var *val1, const struct var *val2));
shared void aforeach(struct var *arr, void (*cb)(size_t idx, struct var *val));

shared struct var *onew();
#define odeclare(a) vdeclare(a, onew())
#define oassign(a) vassign(a, onew())
shared void oclear(struct var *obj);
shared void ofree(struct var *obj);
shared size_t olength(struct var *obj);
shared void oput_s(struct var *obj, const char *key, size_t klen, struct var *val);
shared void oput(struct var *obj, const char *key, struct var *val);
shared struct var **oget_p(struct var *obj, const char *key, size_t klen);
shared struct var *oget_s(struct var *obj, const char *key, size_t klen);
shared struct var *oget(struct var *obj, const char *key);
shared void odelete_p(struct var *obj, struct var **pval);
shared void odelete_s(struct var *obj, const char *key, size_t klen);
shared void odelete(struct var *obj, const char *key);
shared void oforeach_p(struct var *obj, void (*cb)(const char *key, size_t klen, struct var **pval));
shared void oforeach(struct var *obj, void (*cb)(const char *key, size_t klen, struct var *val));

shared struct var *vtojson(struct var *variable);
shared const char *tojson(struct var *variable);
shared struct var *vfromjson_s(const char *jsonstr, size_t jsonslen);
shared struct var *vfromjson(const char *jsonstr);

//
// unicode.c
//

shared struct var *freadall(FILE *stream);
shared void freadlines(FILE *stream, void (*cb)(const char *str, size_t slen));
shared void freadunichars(FILE *stream, void (*cb)(uint32_t unichar));
shared size_t fwriteunichars(FILE *stream, const uint32_t *unichars, size_t nchars);
shared size_t fwriteunichar(FILE *stream, uint32_t unichar);

#endif
