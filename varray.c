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
