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

#define str(a) #a
#define xstr(a) str(a)

#define lambda(ret, args, body) ({ ret f args body &f; })

#define max(a, b) \
    ({ __auto_type _a = (a); \
       __auto_type _b = (b); \
     _a > _b ? _a : _b; })
#define min(a, b) \
    ({ __auto_type _a = (a); \
       __auto_type _b = (b); \
     _a < _b ? _a : _b; })

shared void stacktrace();

// 参考 https://stackoverflow.com/questions/55417186/is-this-a-valid-way-of-checking-if-a-variadic-macro-argument-list-is-empty
#define is_empty(...) (sizeof((char[]){#__VA_ARGS__}) == 1)
// 参考 https://stackoverflow.com/questions/27049491/can-c-c-preprocessor-macros-have-default-parameter-values
#define first_arg(a, ...) a
#define arg_default(a, ...) first_arg(__VA_ARGS__ __VA_OPT__(, ) a) // 确保errno = 后面始终有个值，否则语法错误

// 如果cond为真则退出程序，可选参数为errno，如无则保留当前值不变
#define exitif(cond, ...)                                                         \
    do {                                                                          \
        if (cond) {                                                               \
            if (!is_empty(__VA_ARGS__)) {                                         \
                errno = arg_default(0, __VA_ARGS__);                              \
            }                                                                     \
            fprintf(stderr, "%s,%d %s: \"%s\" %s, exit(%d)\n",                    \
                    __FILE__, __LINE__, __func__, #cond, strerror(errno), errno); \
            stacktrace();                                                         \
            exit(errno);                                                          \
        }                                                                         \
    } while (0)

// 如果cond为真则函数返回，可选参数为errno，如无则保留当前值不变
// 统一约定需要判断成功/失败的函数，返回类型为errno_t，0为成功，其它值为错误码
#define returnif(cond, ...)                                                       \
    do {                                                                          \
        if (cond) {                                                               \
            if (!is_empty(__VA_ARGS__)) {                                         \
                errno = arg_default(0, __VA_ARGS__);                              \
            }                                                                     \
            fprintf(stderr, "%s,%d %s: \"%s\" %s, return %d\n",                   \
                    __FILE__, __LINE__, __func__, #cond, strerror(errno), errno); \
            return errno;                                                         \
        }                                                                         \
    } while (0)

shared void free_s(void **pp);
shared void alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz);

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
        free_s((void **)&(pb->address));                          \
        pb->capacity = 0;                                         \
        pb->length = 0;                                           \
    }
#define generic_buffer_push_function_declaration(sname, fnprefix, type) \
    void fnprefix##push(struct sname *pb, type v)
#define generic_buffer_push_function_definition(sname, fnprefix, type)        \
    generic_buffer_push_function_declaration(sname, fnprefix, type) {         \
        exitif(pb == NULL, EINVAL);                                           \
        size_t newlen = pb->length + 1;                                       \
        size_t newcap = pb->capacity;                                         \
        while (newcap < newlen) {                                             \
            newcap = newcap == 0 ? 1 : newcap << 1;                           \
            exitif(newcap == 0, ERANGE);                                      \
        }                                                                     \
        alloc_s((void **)&(pb->address), pb->capacity, newcap, sizeof(type)); \
        pb->capacity = newcap;                                                \
        (pb->address)[pb->length] = v;                                        \
        pb->length = newlen;                                                  \
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

shared void vdump(const char *prefix, const struct var *pv);
shared void wdump(const char *suffix);
#define dump() wdump(__FILE__ "(" xstr(__LINE__) ")")

shared void gc();

shared void refer(struct var **ppv, char *descr, struct var *pval);
// a = b
// 注意GCC里的__func__ __FUNCTION__ __PRETTY_FUNCTION__是变量名不是字面量，无法用在宏里面字符串连接
// 注意side effect，b如果是表达式避免执行两次
#define vassign(a, b)                                            \
    do {                                                         \
        struct var *_b = (b);                                    \
        exitif(_b == NULL, EINVAL);                              \
        refer(&(a), __FILE__ "," xstr(__LINE__) "," str(a), _b); \
    } while (0)
// var a = b
#define vdeclare(a, b)    \
    struct var *a = NULL; \
    vassign(a, b)

struct var *vnew();
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
shared char *svalue(struct var *pv);
shared size_t slength(struct var *pv);
shared struct var *sconcat(size_t num, ...);

shared struct var *anew(size_t num, ...);
#define adeclare(a) vdeclare(a, anew(0))
#define aassign(a) vassign(a, anew(0))
shared void aclear(struct var *pv);
shared size_t alength(struct var *pv);
shared void apush(struct var *pv, struct var *pval);
shared struct var *apop(struct var *pv);
shared void aput(struct var *pv, size_t idx, struct var *pval);
shared struct var *aget(struct var *pv, size_t idx);
shared void asort(struct var *pv, int (*comp)(const struct var *, const struct var *));
shared void aforeach(struct var *arr, void (*cb)(size_t i, struct var *v));
// #define aforeach(i, v, arr, stmt)                           \
//     do {                                                    \
//         exitif(arr == NULL, EINVAL);                        \
//         exitif(arr->type != vtarray, EINVAL);               \
//         for (size_t i = 0; i < (arr->avalue).length; i++) { \
//             struct var *v = (arr->avalue).address[i];       \
//             stmt;                                           \
//         }                                                   \
//     } while (0)

shared struct var *onew();
#define odeclare(a) vdeclare(a, onew())
#define oassign(a) vassign(a, onew())
shared void oclear(struct var *pv);
shared size_t olength(struct var *pv);
shared void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval);
shared void oput(struct var *pv, const char *key, struct var *pval);
shared struct var *oget_s(struct var *pv, const char *key, size_t klen);
shared struct var *oget(struct var *pv, const char *key);
shared void oforeach(struct var *obj, void (*cb)(const char *k, size_t klen, struct var *v));
// #define oforeach(k, klen, v, obj, stmt)                          \
//     do {                                                         \
//         exitif(obj == NULL, EINVAL);                             \
//         exitif(obj->type != vtobject, EINVAL);                   \
//         for (size_t _i = 0; _i < (obj->ovalue).capacity; _i++) { \
//             char *k = (obj->ovalue).address[_i].key.address;     \
//             size_t klen = (obj->ovalue).address[_i].key.length;  \
//             struct var *v = (obj->ovalue).address[_i].value;     \
//             if (k) {                                             \
//                 stmt;                                            \
//             }                                                    \
//         }                                                        \
//     } while (0)

shared struct var *vtojson(struct var *pv);
#define tojson(pv) svalue(vtojson(pv))

#endif
