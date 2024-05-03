/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

#include <backtrace.h>
#include <backtrace-supported.h>

static struct backtrace_state *bt_state = NULL;

// 參考backtrace_print源代码，error_callback如果不设置会导致release版本segment fault
static void btcb_error(void *data, const char *msg, int errnum) {
    if (errnum > 0) {
        fprintf(stderr, "%s (%d,%s)\n", msg, errnum, strerror(errnum));
    } else {
        fprintf(stderr, "%s\n", msg);
    }
}

static int btcb_full(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
    fprintf(stderr, "%s,%d %s\n", filename ?: "?", lineno, function ?: "?");
    return 0;
}

// static void btcb_syminfo(void *data, uintptr_t pc, const char *symname, uintptr_t symval, uintptr_t symsize) {
//     fprintf(stderr, "%s %p %lld\n", symname, (void *)symval, symsize);
// }

static int btcb_simple(void *data, uintptr_t pc) {
    fprintf(stderr, "\t%p ", (void *)pc);
    backtrace_pcinfo(bt_state, pc, btcb_full, btcb_error, NULL);
    // backtrace_syminfo(bt_state, pc, btcb_syminfo, btcb_error, NULL);
    return 0;
}

void stacktrace() {
    if (!bt_state) {
        bt_state = backtrace_create_state(NULL, BACKTRACE_SUPPORTS_THREADS, btcb_error, NULL);
    };
    backtrace_simple(bt_state, 1, btcb_simple, btcb_error, NULL);
    fprintf(stderr, "\n");
}

// 安全释放内存，并置NULL
void free_s(void **pp) {
    exitif(pp == NULL, EINVAL);
    if (*pp) {
        free(*pp);
        *pp = NULL;
    }
}

// 安全分配/重新分配内存，新分配空间置0
// size_t alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz)
// 约定*pp和oldlen初始值都必须为NULL/0，才能正确运行
// 参考 https://stackoverflow.com/questions/2141277/how-to-zero-out-new-memory-after-realloc
// 注意memset里面void *类型的加法操作结果异常，经测试是会把加数放大16倍，单写程序却无法复现，经查void *类型的算术运算是非法的，参见 https://stackoverflow.com/questions/3523145/pointer-arithmetic-for-void-pointer-in-c
void alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz) {
    // logdebug("oldnum=%lld newnum=%lld sz=%lld", oldnum, newnum, sz);
    exitif(pp == NULL, EINVAL);
    exitif((*pp != NULL) && (oldnum == 0), EINVAL);
    exitif((*pp == NULL) && (oldnum > 0), EINVAL);
    exitif(sz == 0, EINVAL);
    size_t oldsize = oldnum * sz;
    size_t newsize = newnum * sz;
    if (newsize == oldsize) { // 进一步防止诸如sbappend_s每次都调用alloc_s的逻辑错误
        return;
    }
    if (newsize > 0) {
        if (*pp) {
            *pp = realloc(*pp, newsize);
        } else {
            *pp = malloc(newsize);
        }
        exitif(*pp == NULL, ENOMEM);
        if (newsize > oldsize) {
            memset((char *)*pp + oldsize, 0, newsize - oldsize);
        }
    } else {
        free_s(pp);
    }
}

void sbinit(struct stringbuffer *psb) {
    memset(psb, 0, sizeof *psb);
    size_t newcap = 1 << buffer_initial_capacity_exponent;
    alloc_s((void **)&(psb->base), psb->capacity, newcap, 1);
    psb->capacity = newcap;
}

void sbclear(struct stringbuffer *psb) {
    exitif(psb == NULL, EINVAL);
    free_s((void **)&(psb->base));
    psb->capacity = 0;
    psb->length = 0;
}

void sbappend_s(struct stringbuffer *psb, const char *s, size_t slen) {
    // logdebug("*s=%c slen=%lld", *s, slen);
    exitif(psb == NULL, EINVAL);
    exitif((s == NULL) && (slen > 0), EINVAL);
    if (slen == 0) { // 比如json处理转义符的代码，很多长度0的情况
        return;
    }
    size_t newlen = psb->length + slen;
    size_t reqcap = newlen + 1;
    if (psb->capacity < reqcap) { // 修正每次都调用alloc_s的逻辑错误
        size_t newcap = psb->capacity;
        while (newcap < reqcap) {
            newcap = newcap == 0 ? 1 : newcap << 1;
            exitif(newcap == 0, ERANGE);
        }
        alloc_s((void **)&(psb->base), psb->capacity, newcap, 1);
        psb->capacity = newcap;
    }
    memcpy(psb->base + psb->length, s, slen);
    psb->length = newlen;
}

void sbappend(struct stringbuffer *psb, const char *sz) {
    exitif(psb == NULL, EINVAL);
    exitif(sz == NULL, EINVAL);
    sbappend_s(psb, sz, strlen(sz));
}

void sbdump(struct stringbuffer *psb) {
    printf("%lld,%lld,%s", psb->capacity, psb->length, psb->base);
}

void vbinit(struct varbuffer *pb) {
    memset(pb, 0, sizeof *pb);
    size_t newcap = 1 << buffer_initial_capacity_exponent;
    alloc_s((void **)&(pb->base), pb->capacity, newcap, sizeof(struct var *));
    pb->capacity = newcap;
}

void vbclear(struct varbuffer *pb) {
    exitif(pb == NULL, EINVAL);
    free_s((void **)&(pb->base));
    pb->capacity = 0;
    pb->length = 0;
}

void vbpush(struct varbuffer *pb, struct var *v) {
    // logdebug("");
    exitif(pb == NULL, EINVAL);
    size_t newlen = pb->length + 1;
    if (pb->capacity < newlen) { // 修正每次都调用alloc_s的逻辑错误
        size_t newcap = pb->capacity;
        while (newcap < newlen) {
            newcap = newcap == 0 ? 1 : newcap << 1;
            exitif(newcap == 0, ERANGE);
        }
        alloc_s((void **)&(pb->base), pb->capacity, newcap, sizeof(struct var *));
        pb->capacity = newcap;
    }
    (pb->base)[pb->length] = v;
    pb->length = newlen;
}

struct var *vbpop(struct varbuffer *pb) {
    exitif(pb == NULL, EINVAL);
    exitif(pb->length == 0, ERANGE);
    pb->length--;
    struct var *ret = (pb->base)[pb->length];
    memset(&((pb->base)[pb->length]), 0, sizeof(struct var *));
    return ret;
}

void vbput(struct varbuffer *pb, struct var *v, size_t i) {
    exitif(pb == NULL, EINVAL);
    exitif(i >= pb->length, ERANGE);
    (pb->base)[i] = v;
}

struct var *vbget(struct varbuffer *pb, size_t i) {
    exitif(pb == NULL, EINVAL);
    exitif(i >= pb->length, ERANGE);
    return (pb->base)[i];
}

// 查找失败返回-1
ssize_t vbfind(struct varbuffer *pb, struct var *v) {
    exitif(pb == NULL, EINVAL);
    for (size_t i = 0; i < pb->length; i++) {
        if (0 == memcmp(&((pb->base)[i]), &v, sizeof(struct var *))) {
            return i;
        }
    }
    return -1;
}

void vbdump(struct varbuffer *pb) {
    printf("capacity: %lld\n", pb->capacity);
    printf("  length: %lld\n", pb->length);
    for (int i = 0; i < pb->length; i++) {
        printf("%8d: ", i);
        vdump("", (pb->base)[i]);
    }
    printf("\n");
}

// 全局变量取名尽量不用缩写，以便与函数内参数名区分
// 以链表方式存储，如果用数组方式，gc收缩数组还需要修改pointer地址，太麻烦
static __thread struct var *pvarroot = NULL;
// 也以链表方式存储，不会太多，无需优化
static __thread struct ref *prefroot = NULL;

// null/true/false共用一个值以节省内存，共用值不挂在链表上
static const struct var varnull = {.inuse = true, .type = vtnull};
static const struct var vartrue = {.inuse = true, .type = vtboolean, .bvalue = true};
static const struct var varfalse = {.inuse = true, .type = vtboolean, .bvalue = false};

void vdump(const char *prefix, const struct var *pv) {
    // 应该用%p而非%08x，因为地址空间非常大，栈由底递增，堆由顶递减
    printf("%s%p %d ", prefix, pv, pv->inuse);
    switch (pv->type) {
    case vtnull:
        printf("z null");
        break;
    case vtboolean:
        printf("b %s", pv->bvalue ? "true" : "false");
        break;
    case vtnumber:
        printf("n %lg", pv->nvalue);
        break;
    case vtstring:
        printf("s %lld,%lld \"%s\"", (pv->svalue).capacity, (pv->svalue).length, (pv->svalue).base);
        break;
    case vtarray:
        printf("a %lld,%lld [", (pv->avalue).capacity, (pv->avalue).length);
        for (size_t i = 0; i < (pv->avalue).capacity; i++) {
            if (i > 0) {
                printf(" ");
            }
            printf("%p", (pv->avalue).base[i]);
        }
        printf("]");
        break;
    case vtobject:
        printf("o %lld,%lld {", (pv->ovalue).capacity, (pv->ovalue).length);
        for (size_t i = 0; i < (pv->ovalue).capacity; i++) {
            if (i > 0) {
                printf(" ");
            }
            if ((pv->ovalue).base[i].key.base) {
                sbdump(&((pv->ovalue).base[i].key));
                printf(":");
            }
            printf("%p", (pv->ovalue).base[i].value);
        }
        printf("}");
        break;
    default:
        printf("?");
        break;
    }
    printf("\n");
}

void rdump(const char *prefix, const struct ref *pr) {
    printf("%s%p   r %p->%p,%p %p,%p %s\n", prefix, pr, pr->addr, *(pr->addr), pr->value, pr->next, pr->prev, pr->descr);
}

void wdump(const char *suffix) {
    printf("%s(%d) %s(): %s\n", __FILE__, __LINE__, __func__, suffix);
    vdump("     varnull: ", &varnull);
    vdump("     vartrue: ", &vartrue);
    vdump("    varfalse: ", &varfalse);
    for (struct var *pv = pvarroot; pv; pv = pv->next) {
        if (pv == pvarroot) {
            vdump("    pvarroot: ", pv);
        } else {
            vdump("              ", pv);
        }
    }
    for (struct ref *pr = prefroot; pr; pr = pr->next) {
        if (pr == prefroot) {
            rdump("    prefroot: ", pr);
        } else {
            rdump("              ", pr);
        }
    }
    printf("\n");
}

// array和object递归，碰到inuse为1的意味着套娃了，不递归
void markinuse(struct var *pv) {
    // 修改全局常量会程序死掉
    if (pv == &varnull || pv == &vartrue || pv == &varfalse) {
        return;
    }
    pv->inuse = true;
    if (vtarray == pv->type) {
        for (size_t i = 0; i < (pv->avalue).length; i++) {
            struct var *p = (pv->avalue).base[i];
            if (p->inuse) {
                continue;
            }
            markinuse(p);
        }
    } else if (vtobject == pv->type) {
        for (size_t i = 0; i < (pv->ovalue).capacity; i++) {
            struct objnode *pon = &((pv->ovalue).base[i]);
            if ((pon->key).base == NULL) {
                continue;
            }
            struct var *p = pon->value;
            if (p->inuse) {
                continue;
            }
            markinuse(p);
        }
    }
}

// 垃圾回收
void gc() {
    // 删掉登记指向地址和实际指向地址不匹配的引用
    for (struct ref *pr = prefroot; pr;) {
        if (*(pr->addr) == pr->value) {
            pr = pr->next;
        } else {
            struct ref *p = pr;
            if (pr == prefroot) {
                // 必定pr->prev == NULL
                prefroot = pr->next;
            } else {
                // 必定pr->prev != NULL
                pr->prev->next = pr->next;
            }
            if (pr->next) {
                pr->next->prev = pr->prev;
            }
            pr = pr->next;
            free_s((void **)&p);
        }
    }
    // 将所有inuse置0
    for (struct var *pv = pvarroot; pv; pv = pv->next) {
        pv->inuse = false;
    }
    // 从引用开始递归把inuse置1
    for (struct ref *pr = prefroot; pr; pr = pr->next) {
        markinuse(pr->value);
    }
    // 删除inuse为0的，参考删ref的写法
    // 对于array/object不需要递归进去，因为里面的值也是托管的（除了object的key需要手工释放）
    for (struct var *pv = pvarroot; pv;) {
        if (pv->inuse) {
            pv = pv->next;
        } else {
            switch (pv->type) {
            case vtstring:
                sbclear(&(pv->svalue));
                break;
            case vtarray:
                aclear(pv);
                break;
            case vtobject:
                oclear(pv);
                break;
            default:
                break;
            }
            struct var *p = pv;
            if (pv == pvarroot) {
                pvarroot = pv->next;
            } else {
                pv->prev->next = pv->next;
            }
            if (pv->next) {
                pv->next->prev = pv->prev;
            }
            pv = pv->next;
            free_s((void **)&p);
        }
    }
}

// 新建引用，以及赋值操作
// 引用很有可能重复，比如函数内的var()宏，多次调用该函数肯定是重复的，不同函数的栈地址也会重复，所以此处的descr只能表示“最新的”
// TODO: 比如array/object内部创建怎么办？标记-清除算法
void refer(struct var **ppv, char *descr, struct var *pval) {
    exitif(ppv == NULL, EINVAL);
    exitif(descr == NULL, EINVAL);
    exitif(pval == NULL, EINVAL);
    // 记录下（或查询已记录的）该引用的信息，包括地址、值等
    // TODO: 需要加速吗？静态变量并不会太多，应该不需要
    struct ref *pr;
    for (pr = prefroot; pr; pr = pr->next) {
        if (pr->addr == ppv) {
            break;
        }
    }
    if (!pr) {
        alloc_s((void **)&pr, 0, 1, sizeof(struct ref));
        pr->addr = ppv;
        pr->next = prefroot;
        prefroot = pr;
        if (pr->next) {
            pr->next->prev = pr;
        }
    }
    pr->descr = descr;
    pr->value = pval;
    *ppv = pval;
    // printf("%X %X %X %X\n", pr->addr, ppv, *ppv, pval);
}

// 新建空var并挂到pvarroot上
// var一旦创建，禁止修改（object和array也是创建新的，但是其中的修改有对应操作）
struct var *vnew() {
    struct var *pv = NULL;
    alloc_s((void **)&pv, 0, 1, sizeof(struct var));
    pv->next = pvarroot;
    pvarroot = pv;
    if (pv->next) {
        pv->next->prev = pv;
    }
    return pv;
}

enum vtype vtype(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    return pv->type;
}

// 新建null，实际上并不新建
struct var *znew() {
    return (struct var *)&varnull;
}

// 新建boolean，实际上并不新建
struct var *bnew(bool b) {
    return b ? (struct var *)&vartrue : (struct var *)&varfalse;
}

bool bvalue(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtboolean, EINVAL);
    return pv->bvalue;
}

// 新建number
struct var *nnew(double n) {
    struct var *pv = vnew();
    pv->type = vtnumber;
    pv->nvalue = n;
    return pv;
}

double nvalue(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtnumber, EINVAL);
    return pv->nvalue;
}
