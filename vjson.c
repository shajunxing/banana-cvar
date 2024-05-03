/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

static void sbappendescape_s(struct stringbuffer *psb, const char *s, size_t slen) {
    char *base = (char *)s;
    size_t offs = 0;
    size_t offset = 0;
    for (; offset < slen; offset++) {
        char c = *(s + offset);
        char *subs = NULL;
        switch (c) {
        case '"':
            subs = "\\\"";
            break;
        case '\\':
            subs = "\\\\";
            break;
        case '\b':
            subs = "\\b";
            break;
        case '\f':
            subs = "\\f";
            break;
        case '\n':
            subs = "\\n";
            break;
        case '\r':
            subs = "\\r";
            break;
        case '\t':
            subs = "\\t";
            break;
        default:
            break;
        }
        if (subs) {
            // logdebug("*(base+offs)=%c offset-offs=%lld", *(base + offs), offset - offs);
            sbappend_s(psb, base + offs, offset - offs);
            sbappend(psb, subs);
            offs = offset + 1;
        }
    }
    // logdebug("*(base+offs)=%c offset-offs=%lld", *(base + offs), offset - offs);
    sbappend_s(psb, base + offs, offset - offs);
}

static void toj(struct stringbuffer *psb, struct varbuffer *pvb, struct var *pv) {
    vbpush(pvb, pv);
    switch (pv->type) {
    case vtnull:
        sbappend(psb, "null");
        break;
    case vtboolean:
        sbappend(psb, pv->bvalue ? "true" : "false");
        break;
    case vtnumber:
        char *s = NULL;
        int slen = asprintf(&s, "%lg", pv->nvalue);
        sbappend_s(psb, s, slen);
        free_s((void **)&s);
        break;
    case vtstring:
        sbappend(psb, "\"");
        sbappendescape_s(psb, (pv->svalue).base, (pv->svalue).length);
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
            sbappendescape_s(psb, (pv->ovalue).base[i].key.base, (pv->ovalue).base[i].key.length);
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
    struct stringbuffer sb;
    sbinit(&sb);
    struct varbuffer vb;
    vbinit(&vb);
    toj(&sb, &vb, pv);
    vbclear(&vb);
    struct var *ps = vnew();
    ps->type = vtstring;
    // 此处sb不用clear，交由ps托管
    ps->svalue = sb;
    return ps;
}

static bool isstartingnumchar(char ch) {
    return ch == '-' || (ch >= '0' && ch <= '9');
}

static bool isfollowingnumchar(char ch) {
    return isstartingnumchar(ch) || ch == '.' || ch == 'E' || ch == 'e' || ch == '+';
}

struct var *vfromjson_s(const char *jsonstr, size_t jsonslen) {
    struct varbuffer vb;
    vbinit(&vb);
    char *base = (char *)jsonstr;
    for (size_t offset = 0; offset < jsonslen; offset++) {
        char *p = base + offset;
        // 栈顶元素是否已完整，如是则在循环尾部尝试向下合并
        // 每次循环开始重置为false
        bool trymerge = false;
        // vbdump(&vb);
        // printf("%c\n\n", *p);
        if (*p == '{') {
            vbpush(&vb, onew());
        } else if (*p == '[') {
            vbpush(&vb, anew(0));
        } else if (*p == '"') {
            // 性能测试结果alloc_s巨量调用和时间占用，此处给一个较大的初始容量
            struct var *s = vnew();
            s->type = vtstring;
            struct stringbuffer *sb = &(s->svalue);
            sbinit(sb);
            // 注意空字符串
            size_t offs = offset + 1; // 记录非转义符片段的起始位置，sbappend bulk copy提速
            for (offset++; offset < jsonslen; offset++) {
                p = base + offset;
                if (*p == '"') {
                    // logdebug("*(base+offs)=%c offset-offs=%lld", *(base + offs), offset - offs);
                    sbappend_s(sb, base + offs, offset - offs); // 拷贝之前的非转义片段
                    break;
                } else if (*p == '\\') {
                    // logdebug("*(base+offs)=%c offset-offs=%lld", *(base + offs), offset - offs);
                    sbappend_s(sb, base + offs, offset - offs); // 拷贝之前的非转义片段
                    offset++;
                    if (offset < jsonslen) {
                        p = base + offset;
                        if (*p == 'b') {
                            sbappend(sb, "\b");
                        } else if (*p == 'f') {
                            sbappend(sb, "\f");
                        } else if (*p == 'n') {
                            sbappend(sb, "\n");
                        } else if (*p == 'r') {
                            sbappend(sb, "\r");
                        } else if (*p == 't') {
                            sbappend(sb, "\t");
                        } else if (*p == 'u') {
                            // 完整的utf16（变长的）支持太麻烦，而且没什么用，跳过吧
                            sbappend(sb, "\\u");
                        } else {
                            sbappend_s(sb, p, 1); // 包括"/\等其它的都原样录入
                        }
                    }
                    offs = offset + 1; // 新起始位置
                }
            }
            vbpush(&vb, s);
            trymerge = true;
        } else if (isstartingnumchar(*p)) {
            struct stringbuffer sb;
            sbinit(&sb);
            size_t left = offset;
            for (offset++; offset < jsonslen; offset++) {
                if (!isfollowingnumchar(*(base + offset))) {
                    offset--;
                    break;
                }
            }
            sbappend_s(&sb, p, offset + 1 - left);
            vbpush(&vb, nnew(atof(sb.base)));
            sbclear(&sb);
            trymerge = true;
        } else if (*p == 't') {
            offset += 3;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "rue", 3) == 0) {
                    vbpush(&vb, bnew(true));
                }
            }
            trymerge = true;
        } else if (*p == 'f') {
            offset += 4;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "alse", 4) == 0) {
                    vbpush(&vb, bnew(false));
                }
            }
            trymerge = true;
        } else if (*p == 'n') {
            offset += 3;
            if (offset < jsonslen) {
                if (strncmp(p + 1, "ull", 3) == 0) {
                    vbpush(&vb, znew());
                }
            }
            trymerge = true;
        } else if (*p == ']' || *p == '}') {
            trymerge = true;
        }
        // 跳过别的字符，主要是空格，其它权当容错
        // 尝试向下合并，并不判断逗号，只要数据完整了就执行
        if (trymerge) {
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
    }
    // vbdump(&vb);
    struct var *result = vbget(&vb, 0);
    vbclear(&vb);
    return result;
}

struct var *vfromjson(const char *jsonstr) {
    return vfromjson_s(jsonstr, strlen(jsonstr));
}