/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef VAR_H
#define VAR_H

#include <stdarg.h>
#include <stdbool.h>
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

#define exitif(cond, errnum)                         \
    do {                                             \
        if (cond) {                                  \
            fprintf(stderr, "%s, %s(%d) %s(): %s\n", \
                    strerror(errnum),                \
                    __FILE__, __LINE__, __func__,    \
                    #cond);                          \
            exit(errnum);                            \
        }                                            \
    } while (0)

// 安全分配/重新分配内存，新分配空间置0，出错退出程序，注意必须用宏，否则错误信息看不到发生的位置
// size_t safealloc(void **pp, size_t oldnum, size_t newnum, size_t sz)
// 约定*pp和oldlen初始值都必须为NULL/0，才能正确运行
// 参考 https://stackoverflow.com/questions/2141277/how-to-zero-out-new-memory-after-realloc
#define safefree(pp)                  \
    do {                              \
        void **__pp = (void **)(pp);  \
        exitif(__pp == NULL, EINVAL); \
        if (*__pp) {                  \
            free(*__pp);              \
            *__pp = NULL;             \
        }                             \
    } while (0)
// 注意memset里面void *类型的加法操作结果异常，经测试是会把加数放大16倍，单写程序却无法复现，经查void *类型的算术运算是非法的，参见 https://stackoverflow.com/questions/3523145/pointer-arithmetic-for-void-pointer-in-c
#define safealloc(pp, oldnum, newnum, sz) ({                           \
    void **__pp = (void **)(pp);                                       \
    size_t __oldnum = oldnum;                                          \
    size_t __newnum = newnum;                                          \
    size_t __sz = sz;                                                  \
    exitif(__pp == NULL, EINVAL);                                      \
    exitif((*__pp != NULL) && (__oldnum == 0), EINVAL);                \
    exitif((*__pp == NULL) && (__oldnum > 0), EINVAL);                 \
    exitif(__sz == 0, EINVAL);                                         \
    size_t __oldsize = __oldnum * __sz;                                \
    size_t __newsize = __newnum * __sz;                                \
    if (__newsize > 0) {                                               \
        if (*pp) {                                                     \
            *pp = realloc(*pp, __newsize);                             \
        } else {                                                       \
            *pp = malloc(__newsize);                                   \
        }                                                              \
        exitif(*pp == NULL, ENOMEM);                                   \
        if (__newsize > __oldsize) {                                   \
            memset((char *)*pp + __oldsize, 0, __newsize - __oldsize); \
        }                                                              \
    } else {                                                           \
        safefree(pp);                                                  \
    }                                                                  \
    newnum;                                                            \
})

#define auto __auto_type
#define max(a, b) \
    ({ auto __a = (a); \
       auto __b = (b); \
     __a > __b ? __a : __b; })
#define min(a, b) \
    ({ auto __a = (a); \
       auto __b = (b); \
     __a < __b ? __a : __b; })

// 字符串buffer类型，与通用buffer类型有区别
struct stringbuffer {
    char *address;
    size_t capacity;
    size_t length;
};
#define sbdeclare(var_name) struct stringbuffer var_name = {NULL, 0, 0}
void sbclear(struct stringbuffer *psb);
void sbappend_s(struct stringbuffer *psb, const char *s, size_t slen);
void sbappend(struct stringbuffer *psb, const char *sz);
void sbdump(struct stringbuffer *psb);

// 通用buffer类型
#define generic_buffer_struct_declaration(name, type) \
    struct name {                                     \
        type *address;                                \
        size_t capacity;                              \
        size_t length;                                \
    }
#define generic_buffer_variable_declaration(sname, vname) struct sname vname = {NULL, 0, 0}
#define generic_buffer_clear_function_declaration(sname, fnprefix) \
    void fnprefix##clear(struct sname *pb)
#define generic_buffer_clear_function_definition(sname, fnprefix) \
    generic_buffer_clear_function_declaration(sname, fnprefix) {  \
        exitif(pb == NULL, EINVAL);                               \
        safefree(&(pb->address));                                 \
        pb->capacity = 0;                                         \
        pb->length = 0;                                           \
    }
#define generic_buffer_push_function_declaration(sname, fnprefix, type) \
    void fnprefix##push(struct sname *pb, type v)
#define generic_buffer_push_function_definition(sname, fnprefix, type)                \
    generic_buffer_push_function_declaration(sname, fnprefix, type) {                 \
        exitif(pb == NULL, EINVAL);                                                   \
        size_t newlen = pb->length + 1;                                               \
        size_t newcap = pb->capacity;                                                 \
        while (newcap < newlen) {                                                     \
            newcap = newcap == 0 ? 1 : newcap << 1;                                   \
            exitif(newcap == 0, ERANGE);                                              \
        }                                                                             \
        pb->capacity = safealloc(&(pb->address), pb->capacity, newcap, sizeof(type)); \
        (pb->address)[pb->length] = v;                                                \
        pb->length = newlen;                                                          \
    }
#define generic_buffer_pop_function_declaration(sname, fnprefix, type) \
    type fnprefix##pop(struct sname *pb)
#define generic_buffer_pop_function_definition(sname, fnprefix, type) \
    generic_buffer_pop_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(pb->length == 0, ERANGE);                              \
        pb->length--;                                                 \
        type ret = (pb->address)[pb->length];                         \
        memset(&((pb->address)[pb->length]), 0, sizeof(type));        \
        return ret;                                                   \
    }
#define generic_buffer_put_function_declaration(sname, fnprefix, type) \
    void fnprefix##put(struct sname *pb, type v, size_t i)
#define generic_buffer_put_function_definition(sname, fnprefix, type) \
    generic_buffer_put_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(i >= pb->length, ERANGE);                              \
        (pb->address)[i] = v;                                         \
    }
#define generic_buffer_get_function_declaration(sname, fnprefix, type) \
    type fnprefix##get(struct sname *pb, size_t i)
#define generic_buffer_get_function_definition(sname, fnprefix, type) \
    generic_buffer_get_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(i >= pb->length, ERANGE);                              \
        return (pb->address)[i];                                      \
    }
// 查找失败返回-1
#define generic_buffer_find_function_declaration(sname, fnprefix, type) \
    ssize_t fnprefix##find(struct sname *pb, type v)
#define generic_buffer_find_function_definition(sname, fnprefix, type) \
    generic_buffer_find_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                    \
        for (size_t i = 0; i < pb->length; i++) {                      \
            if (0 == memcmp(&((pb->address)[i]), &v, sizeof(type))) {  \
                return i;                                              \
            }                                                          \
        }                                                              \
        return -1;                                                     \
    }

enum vtype {
    vtnull,
    vtboolean,
    vtnumber,
    vtstring,
    vtarray,
    vtobject
};

struct var;

struct objnode {
    struct stringbuffer key;
    struct var *value;
};

generic_buffer_struct_declaration(varbuffer, struct var *);
generic_buffer_clear_function_declaration(varbuffer, vb);
generic_buffer_push_function_declaration(varbuffer, vb, struct var *);
generic_buffer_pop_function_declaration(varbuffer, vb, struct var *);
generic_buffer_put_function_declaration(varbuffer, vb, struct var *);
generic_buffer_get_function_declaration(varbuffer, vb, struct var *);
generic_buffer_find_function_declaration(varbuffer, vb, struct var *);

generic_buffer_struct_declaration(objnodebuffer, struct objnode);

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

#define str(a) #a
#define xstr(a) str(a)

shared void vdump(const char *prefix, const struct var *pv);
shared void wdump(const char *suffix);
#define dump() wdump(__FILE__ "(" xstr(__LINE__) ")")

shared void gc();

shared void refer(struct var **ppv, char *descr, struct var *pval);
// a = b
#define vassign(a, b) refer(&(a), __FILE__ "," xstr(__LINE__) "," str(a), (b))
// var a = null
// 注意GCC里的__func__ __FUNCTION__ __PRETTY_FUNCTION__是变量名不是字面量，无法用在宏里面字符串连接
#define vdeclare(a, b)    \
    struct var *a = NULL; \
    vassign(a, b)

struct var *vnew();
shared enum vtype vtype(struct var *pv);

shared struct var *znew();
#define zdeclare(a) vdeclare(a, znew())

shared struct var *bnew(bool b);
#define bdeclare(a, b) vdeclare(a, bnew(b))
shared bool bvalue(struct var *pv);

shared struct var *nnew(double n);
#define ndeclare(a, b) vdeclare(a, nnew(b))
shared double nvalue(struct var *pv);

shared struct var *snew_s(const char *s, size_t slen);
shared struct var *snew(const char *sz);
#define sdeclare(a, b) vdeclare(a, snew(b))
shared char *svalue(struct var *pv);
shared size_t slength(struct var *pv);
shared struct var *sconcat(size_t num, ...);

shared struct var *anew(size_t num, ...);
#define adeclare(a) vdeclare(a, anew(0))
shared void aclear(struct var *pv);
shared size_t alength(struct var *pv);
shared void apush(struct var *pv, struct var *pval);
shared struct var *apop(struct var *pv);
shared void aput(struct var *pv, size_t idx, struct var *pval);
shared struct var *aget(struct var *pv, size_t idx);
typedef int (*acomp_t)(const struct var *, const struct var *);
shared void asort(struct var *pv, acomp_t comp);

shared struct var *onew();
#define odeclare(a) vdeclare(a, onew())
shared void oclear(struct var *pv);
shared size_t olength(struct var *pv);
shared void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval);
shared void oput(struct var *pv, const char *key, struct var *pval);
shared struct var *oget_s(struct var *pv, const char *key, size_t klen);
shared struct var *oget(struct var *pv, const char *key);

shared struct var *vtojson(struct var *pv);
#define tojson(pv) svalue(vtojson(pv))

#endif
