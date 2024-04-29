/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

static void toj(struct stringbuffer *psb, struct varbuffer *pvb, struct var *pv) {
    vbpush(pvb, pv);
    char buff[128];
    switch (pv->type) {
    case vtnull:
        sbappend(psb, "null");
        break;
    case vtboolean:
        sbappend(psb, pv->bvalue ? "true" : "false");
        break;
    case vtnumber:
        memset(buff, 0, sizeof buff);
        snprintf(buff, sizeof buff, "%lg", pv->nvalue);
        sbappend(psb, buff);
        break;
    case vtstring:
        sbappend(psb, "\"");
        sbappend_s(psb, (pv->svalue).base, (pv->svalue).length);
        sbappend(psb, "\"");
        break;
    case vtarray:
        sbappend(psb, "[");
        for (size_t i = 0; i < (pv->avalue).length; i++) {
            // 防止套娃
            if (vbfind(pvb, (pv->avalue).base[i]) > -1) {
                continue;
            }
            if (i > 0) {
                sbappend(psb, ",");
            }
            toj(psb, pvb, (pv->avalue).base[i]);
        }
        sbappend(psb, "]");
        break;
    case vtobject:
        sbappend(psb, "{");
        size_t j = 0;
        for (size_t i = 0; i < (pv->ovalue).capacity; i++) {
            if ((pv->ovalue).base[i].key.base == NULL) {
                continue;
            }
            // 防止套娃
            if (vbfind(pvb, (pv->ovalue).base[i].value) > -1) {
                continue;
            }
            if (j > 0) {
                sbappend(psb, ",");
            }
            j++;
            sbappend(psb, "\"");
            sbappend_s(psb, (pv->ovalue).base[i].key.base, (pv->ovalue).base[i].key.length);
            sbappend(psb, "\":");
            toj(psb, pvb, (pv->ovalue).base[i].value);
        }
        sbappend(psb, "}");
        break;
    }
    vbpop(pvb);
}

// 转换为JSON兼容的字符串格式
struct var *vtojson(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    sbdeclare(sb);
    struct varbuffer vb = {NULL, 0, 0};
    toj(&sb, &vb, pv);
    vbclear(&vb);
    struct var *ps = vnew();
    ps->type = vtstring;
    // 此处sb不用clear，交由ps托管
    ps->svalue = sb;
    return ps;
}
