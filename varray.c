/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

// 新建array
struct var *anew(size_t num, ...) {
    struct var *pv = vnew();
    pv->type = vtarray;
    va_list args;
    va_start(args, num);
    for (size_t i = 0; i < num; i++) {
        struct var *p = va_arg(args, struct var *);
        exitif(p == NULL, EINVAL);
        vbpush(&(pv->avalue), p);
    }
    va_end(args);
    return pv;
}

void aclear(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    vbclear(&(pv->avalue));
}

size_t alength(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    return (pv->avalue).length;
}

void apush(struct var *pv, struct var *pval) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    exitif(pval == NULL, EINVAL);
    vbpush(&(pv->avalue), pval);
}

struct var *apop(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    return vbpop(&(pv->avalue));
}

void aput(struct var *pv, size_t idx, struct var *pval) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    exitif(pval == NULL, EINVAL);
    vbput(&(pv->avalue), pval, idx);
}

struct var *aget(struct var *pv, size_t idx) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    return vbget(&(pv->avalue), idx);
}

static int acomp_default(const struct var *pv1, const struct var *pv2) {
    exitif(pv1 == NULL, EINVAL);
    exitif(pv2 == NULL, EINVAL);
    if (pv1 == pv2) {
        return 0;
    }
    int typediff = pv1->type - pv2->type;
    if (0 != typediff) {
        return typediff;
    }
    switch (pv1->type) {
    case vtnull:
        return 0;
    case vtboolean:
        return pv1->bvalue - pv2->bvalue;
    case vtnumber:
        return (int)(pv1->nvalue - pv2->nvalue);
    case vtstring:
        // 把\0包含进比较里
        // 参见 https://stackoverflow.com/questions/36518931/what-does-strcmp-return-if-two-similar-strings-are-of-different-lengths
        return memcmp((pv1->svalue).base, (pv2->svalue).base, min((pv1->svalue).length, (pv1->svalue).length) + 1);
    case vtarray:
        return (pv1->avalue).length - (pv2->avalue).length;
    case vtobject:
        return (pv1->ovalue).length - (pv2->ovalue).length;
    }
    return 0;
}

static int acomp_relay(void *ctx, const void *e1, const void *e2) {
    int (*comp)(const struct var *, const struct var *) = (int (*)(const struct var *, const struct var *))ctx;
    const struct var *pv1 = *((const struct var **)e1);
    const struct var *pv2 = *((const struct var **)e2);
    return comp(pv1, pv2);
}

void asort(struct var *pv, int (*comp)(const struct var *, const struct var *)) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtarray, EINVAL);
    if (NULL == comp) {
        comp = acomp_default;
    }
    qsort_s(pv->avalue.base, pv->avalue.length, sizeof(struct var *), acomp_relay, comp);
}

void aforeach(struct var *arr, void (*cb)(size_t i, struct var *v)) {
    exitif(arr == NULL, EINVAL);
    exitif(arr->type != vtarray, EINVAL);
    for (size_t i = 0; i < (arr->avalue).length; i++) {
        struct var *v = (arr->avalue).base[i];
        cb(i, v);
    }
}
