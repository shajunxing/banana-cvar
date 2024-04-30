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
    vbdeclare(vb);
    toj(&sb, &vb, pv);
    vbclear(&vb);
    struct var *ps = vnew();
    ps->type = vtstring;
    // 此处sb不用clear，交由ps托管
    ps->svalue = sb;
    return ps;
}

struct var *fromj(int expect, char *pchar, size_t remain) {
    if (remain <= 0) {
        fprintf(stderr, "Out of chars\n");
        return znew();
    }
    switch (expect) {
    default:
        // 跳过空格
        while ((*pchar == ' ' || *pchar == '\r' || *pchar == '\n' || *pchar == '\t') && remain > 0) {
            pchar++;
            remain--;
        }
        break;
    }
}

static bool isstartingnumchar(char ch) {
    return ch == '-' || (ch >= '0' && ch <= '9');
}

static bool isfollowingnumchar(char ch) {
    return isstartingnumchar(ch) || ch == '.' || ch == 'E' || ch == 'e' || ch == '+';
}

static uint16_t hex2int(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
}

struct var *vfromjson_s(const char *jsonstr, size_t jsonslen) {
    vbdeclare(vb);
    char *base = (char *)jsonstr;
    for (size_t offset = 0; offset < jsonslen; offset++) {
        char *p = base + offset;
        if (*p == '{') {
            vbpush(&vb, onew());
        } else if (*p == '[') {
            vbpush(&vb, anew(0));
        } else if (*p == '"') {
            sbdeclare(sb);
            for (offset++; offset < jsonslen; offset++) {
                p = base + offset;
                if (*p != '"') {
                    if (*p == '\\') {
                        offset++;
                        if (offset < jsonslen) {
                            p = base + offset;
                            if (*p == 'b') {
                                sbappend(&sb, "\b");
                            } else if (*p == 'f') {
                                sbappend(&sb, "\f");
                            } else if (*p == 'n') {
                                sbappend(&sb, "\n");
                            } else if (*p == 'r') {
                                sbappend(&sb, "\r");
                            } else if (*p == 't') {
                                sbappend(&sb, "\t");
                            } else { // utf16不处理了，废弃的东西不支持
                                sbappend_s(&sb, p, 1); // 包括"/\等其它的都原样录入
                            }
                        }
                    } else {
                        sbappend_s(&sb, p, 1);
                    }
                } else {
                    break;
                }
            }
            vbpush(&vb, snew(sb.base));
            sbclear(&sb);
        } else if (isstartingnumchar(*p)) {
            sbdeclare(sb);
            sbappend_s(&sb, p, 1);
            for (offset++; offset < jsonslen; offset++) {
                p = base + offset;
                if (isfollowingnumchar(*p)) {
                    sbappend_s(&sb, p, 1);
                } else {
                    offset--;
                    break;
                }
            }
            vbpush(&vb, nnew(atof(sb.base)));
            sbclear(&sb);
        } else if (*p == 't') {
            offset += 3;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "rue", 3) == 0) {
                    vbpush(&vb, bnew(true));
                }
            }
        } else if (*p == 'f') {
            offset += 4;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "alse", 4) == 0) {
                    vbpush(&vb, bnew(false));
                }
            }
        } else if (*p == 'n') {
            offset += 3;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "ull", 3) == 0) {
                    vbpush(&vb, znew());
                }
            }
        } else if (*p == ',' || *p == ']' || *p == '}') {
            if (vb.length >= 3 && (vb.base[vb.length - 3])->type == vtobject) {
                struct var *v = vbpop(&vb);
                struct var *k = vbpop(&vb);
                if (k->type == vtstring) {
                    oput_s(vb.base[vb.length - 1], (k->svalue).base, (k->svalue).length, v);
                }
            } else if (vb.length >= 2 && (vb.base[vb.length - 2])->type == vtarray) {
                struct var *v = vbpop(&vb);
                apush(vb.base[vb.length - 1], v);
            }
        }
        // 跳过别的字符，主要是空格，其它权当容错
    }
    struct var *result = vbget(&vb, 0);
    vbclear(&vb);
    return result;
}

struct var *vfromjson(const char *jsonstr) {
    return vfromjson_s(jsonstr, strlen(jsonstr));
}