/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

// 新建string
// 注意s需要复制而不是引用
struct var *snew_s(const char *s, size_t slen) {
    exitif((s == NULL) && (slen > 0), EINVAL);
    // 如果长度为0，也会开辟长度1的数组
    struct var *pv = vnew();
    pv->type = vtstring;
    sbappend_s(&(pv->svalue), s, slen);
    return pv;
}

struct var *snew(const char *sz) {
    exitif(sz == NULL, EINVAL);
    return snew_s(sz, strlen(sz));
}

char *svalue(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtstring, EINVAL);
    return (pv->svalue).base;
}

size_t slength(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtstring, EINVAL);
    return (pv->svalue).length;
}

struct var *sconcat(size_t num, ...) {
    struct var *pv = vnew();
    pv->type = vtstring;
    va_list args;
    va_start(args, num);
    for (size_t i = 0; i < num; i++) {
        struct var *p = va_arg(args, struct var *);
        exitif(p == NULL, EINVAL);
        sbappend_s(&(pv->svalue), (p->svalue).base, (p->svalue).length);
    }
    va_end(args);
    return pv;
}

struct var *sformat(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *s = NULL;
    int slen = vasprintf(&s, fmt, args);
    va_end(args);
    struct var *pv = vnew();
    pv->type = vtstring;
    sbappend_s(&(pv->svalue), s, slen);
    free_s(&s);
    return pv;
}