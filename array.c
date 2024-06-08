/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

// 新建array
struct var *anew(size_t len, ...) {
    struct var *arr = vnew();
    arr->type = vtarray;
    vbinit(&(arr->avalue));
    va_list args;
    va_start(args, len);
    for (size_t i = 0; i < len; i++) {
        struct var *p = va_arg(args, struct var *);
        exitif(p == NULL);
        vbpush(&(arr->avalue), p);
    }
    va_end(args);
    return arr;
}

void aclear(struct var *arr) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    memset((arr->avalue).base, 0, (arr->avalue).capacity * sizeof((arr->avalue).base[0]));
    (arr->avalue).length = 0;
}

void afree(struct var *arr) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    vbfree(&(arr->avalue));
}

size_t alength(struct var *arr) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    return (arr->avalue).length;
}

void apush(struct var *arr, struct var *val) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    exitif(val == NULL);
    vbpush(&(arr->avalue), val);
}

struct var *apop(struct var *arr) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    return vbpop(&(arr->avalue));
}

void aput(struct var *arr, size_t idx, struct var *val) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    exitif(val == NULL);
    vbput(&(arr->avalue), val, idx);
}

struct var *aget(struct var *arr, size_t idx) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    return vbget(&(arr->avalue), idx);
}

static int acomp_default(const struct var *val1, const struct var *val2) {
    exitif(val1 == NULL);
    exitif(val2 == NULL);
    if (val1 == val2) {
        return 0;
    }
    int typediff = val1->type - val2->type;
    if (0 != typediff) {
        return typediff;
    }
    switch (val1->type) {
    case vtnull:
        return 0;
    case vtboolean:
        return val1->bvalue - val2->bvalue;
    case vtnumber:
        return (int)(val1->nvalue - val2->nvalue);
    case vtstring:
        // 把\0包含进比较里
        // 参见 https://stackoverflow.com/questions/36518931/what-does-strcmp-return-if-two-similar-strings-are-of-different-lengths
        return memcmp((val1->svalue).base, (val2->svalue).base, min((val1->svalue).length, (val1->svalue).length) + 1);
    case vtarray:
        return (val1->avalue).length - (val2->avalue).length;
    case vtobject:
        return (val1->ovalue).length - (val2->ovalue).length;
    }
    return 0;
}

static int acomp_proxy(void *ctx, const void *e1, const void *e2) {
    int (*comp)(const struct var *, const struct var *) = (int (*)(const struct var *, const struct var *))ctx;
    const struct var *val1 = *((const struct var **)e1);
    const struct var *val2 = *((const struct var **)e2);
    return comp(val1, val2);
}

void asort(struct var *arr, int (*comp)(const struct var *val1, const struct var *val2)) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    if (NULL == comp) {
        comp = acomp_default;
    }
    qsort_s(arr->avalue.base, arr->avalue.length, sizeof(struct var *), acomp_proxy, comp);
}

void aforeach(struct var *arr, void (*cb)(size_t i, struct var *v)) {
    exitif(arr == NULL);
    exitif(arr->type != vtarray);
    for (size_t i = 0; i < (arr->avalue).length; i++) {
        struct var *v = (arr->avalue).base[i];
        cb(i, v);
    }
}
