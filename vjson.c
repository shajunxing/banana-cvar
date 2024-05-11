/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

static void esc(struct stringbuffer *psb, const char *s, size_t slen) {
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

static inline bool isdnum(double d) {
    return d >= 0 && d < 10;
}

static inline char todchar(double d) {
    return isdnum(d) ? (char)d + '0' : '0';
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
        // 自己写，取代asprintf
        // 为了防止数值溢出，不用整型而是用double类型，以及使用math函数计算而非强制类型转换
        double num = pv->nvalue;
        if (num < 0) {
            sbappend(psb, "-");
            num = -num;
        }
        double inte;
        double frac = modf(num, &inte);
        int ilen = 0;
        for (; num >= 1; num /= 10) {
            ilen++;
        }
        // logdebug("inte=%lf frac=%lf ilen=%d", inte, frac, ilen);
        if (ilen == 0) {
            sbappend(psb, "0");
        } else {
            for (int i = ilen - 1; i >= 0; i--) {
                double m = pow(10, i);
                double d = trunc(inte / m) - trunc(inte / (m * 10)) * 10;
                char ch = todchar(d);
                // logdebug("i=%d m=%lf d=%lf %c", i, m, d, ch);
                sbappend_s(psb, &ch, 1);
            }
        }
        double prec = 0.01; // 精度
        if (frac > prec) {
            sbappend(psb, ".");
            for (int i = 0; i < 5 && frac > prec; i++) {
                frac *= 10;
                double fi;
                double ff = modf(frac, &fi);
                // logdebug("frac=%lf fi=%lf ff=%lf", frac, fi, ff);
                char ch = todchar(fi);
                sbappend_s(psb, &ch, 1);
                frac = ff;
            }
        }
        break;
    case vtstring:
        sbappend(psb, "\"");
        esc(psb, (pv->svalue).base, (pv->svalue).length);
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
            esc(psb, (pv->ovalue).base[i].key.base, (pv->ovalue).base[i].key.length);
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
    exitif(pv == NULL);
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

static inline bool isdchar(char ch) {
    return ch >= '0' && ch <= '9';
}

static inline double todnum(char ch) {
    return ch - '0';
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
        } else if (*p == '-' || *p == '.' || isdchar(*p)) { // json没规定.开头，但我这里允许
            // 替换stringbuffer和atof，是否有更好的性能？
            int sign = 1, esign = 1;
            double inte = 0, frac = 0, flen = 0.1; // 不用整型，因为算出来的精度差，另外cpu的浮点运算都很快
            int expo = 0; // 注意pow(10, esign * expo)，一个unsigned和一个sign相乘会导致错误结果，致使pow的值inf
            char expect = 'i'; // 当前期待哪类值
            if (*p == '-') {
                sign = -1;
            } else if (*p == '.') {
                expect = 'f';
            } else {
                inte = *p - '0';
            }
            for (offset++; offset < jsonslen; offset++) {
                p = base + offset;
                if (expect == 'i') {
                    if (isdchar(*p)) {
                        inte = inte * 10 + todnum(*p);
                    } else if (*p == '.') {
                        expect = 'f';
                    } else if (*p == 'E' || *p == 'e') {
                        expect = 'e';
                    } else {
                        break;
                    }
                } else if (expect == 'f') {
                    if (isdchar(*p)) {
                        frac = frac + todnum(*p) * flen;
                        flen *= 0.1;
                    } else if (*p == 'E' || *p == 'e') {
                        expect = 'e';
                    } else {
                        break;
                    }
                } else if (expect == 'e') {
                    if (*p == '+') {
                        continue;
                    } else if (*p == '-') {
                        esign = -1;
                    } else if (isdchar(*p)) {
                        expo = expo * 10 + todnum(*p);
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            offset--; // 往前拨一位，不要超出数字的范围
            double num = sign * (inte + frac) * pow(10, esign * expo);
            // logdebug("sign:%d inte:%lf frac:%lf flen:%lf esign:%d expo:%d num:%lf", sign, inte, frac, flen, esign, expo, num);
            vbpush(&vb, nnew(num));
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
    // dump();
    struct var *result;
    if (vb.length > 0) {
        result = vbget(&vb, 0);
    } else {
        result = znew();
    }
    vbclear(&vb);
    return result;
}

struct var *vfromjson(const char *jsonstr) {
    return vfromjson_s(jsonstr, strlen(jsonstr));
}