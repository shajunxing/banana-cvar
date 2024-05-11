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
    exitif((s == NULL) && (slen > 0));
    // 如果长度为0，也会开辟长度1的数组
    struct var *pv = vnew();
    pv->type = vtstring;
    sbinit(&(pv->svalue));
    sbappend_s(&(pv->svalue), s, slen);
    return pv;
}

struct var *snew(const char *sz) {
    exitif(sz == NULL);
    return snew_s(sz, strlen(sz));
}

const char *svalue(struct var *pv) {
    exitif(pv == NULL);
    exitif(pv->type != vtstring);
    return (const char *)((pv->svalue).base);
}

size_t slength(struct var *pv) {
    exitif(pv == NULL);
    exitif(pv->type != vtstring);
    return (pv->svalue).length;
}

struct var *sconcat(size_t num, ...) {
    struct var *pv = vnew();
    pv->type = vtstring;
    va_list args;
    va_start(args, num);
    for (size_t i = 0; i < num; i++) {
        struct var *p = va_arg(args, struct var *);
        exitif(p == NULL);
        sbappend_s(&(pv->svalue), (p->svalue).base, (p->svalue).length);
    }
    va_end(args);
    return pv;
}

struct var *sformat(const char *fmt, ...) {
    exitif(fmt == NULL);
    va_list args;
    va_start(args, fmt);
    char *ptr;
    int len = vasprintf(&ptr, fmt, args);
    va_end(args);
    exitif(len == -1);
    struct var *pv = vnew();
    pv->type = vtstring;
    (pv->svalue).base = ptr;
    (pv->svalue).length = len;
    (pv->svalue).capacity = len + 1; // 长度不是2的倍数，不过无所谓了
    return pv;
}

// 以下正则表达式实现取自Lua 5.1 lstrlib.c

/******************************************************************************
 * Copyright (C) 1994-2012 Lua.org, PUC-Rio.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#define MAX_CAPTURES 32
#define CAP_UNFINISHED (-1)
#define uchar(c) ((unsigned char)(c))
#define ESC_CHAR '%'

struct match_state {
    const char *src_init; /* init of source string */
    const char *src_end; /* end (`\0') of source string */
    int level; /* total number of captures (finished or unfinished) */
    struct {
        const char *init;
        ptrdiff_t len;
    } capture[MAX_CAPTURES];
};

static int check_capture(struct match_state *ms, int l) {
    l -= '1';
    if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
        logexit("invalid capture index");
    return l;
}

static int capture_to_close(struct match_state *ms) {
    int level = ms->level;
    for (level--; level >= 0; level--)
        if (ms->capture[level].len == CAP_UNFINISHED)
            return level;
    logexit("invalid pattern capture");
}

static const char *class_end(struct match_state *ms, const char *p) {
    switch (*p++) {
    case ESC_CHAR: {
        if (*p == '\0')
            logexit("malformed pattern (ends with '%%')");
        return p + 1;
    }
    case '[': {
        if (*p == '^')
            p++;
        do { /* look for a `]' */
            if (*p == '\0')
                logexit("malformed pattern (missing ']')");
            if (*(p++) == ESC_CHAR && *p != '\0')
                p++; /* skip escapes (e.g. `%]') */
        } while (*p != ']');
        return p + 1;
    }
    default: {
        return p;
    }
    }
}

static int match_class(int c, int cl) {
    int res;
    switch (tolower(cl)) {
    case 'a':
        res = isalpha(c);
        break;
    case 'c':
        res = iscntrl(c);
        break;
    case 'd':
        res = isdigit(c);
        break;
    case 'l':
        res = islower(c);
        break;
    case 'p':
        res = ispunct(c);
        break;
    case 's':
        res = isspace(c);
        break;
    case 'u':
        res = isupper(c);
        break;
    case 'w':
        res = isalnum(c);
        break;
    case 'x':
        res = isxdigit(c);
        break;
    case 'z':
        res = (c == 0);
        break;
    default:
        return (cl == c);
    }
    return (islower(cl) ? res : !res);
}

static int match_bracket_class(int c, const char *p, const char *ec) {
    int sig = 1;
    if (*(p + 1) == '^') {
        sig = 0;
        p++; /* skip the `^' */
    }
    while (++p < ec) {
        if (*p == ESC_CHAR) {
            p++;
            if (match_class(c, uchar(*p)))
                return sig;
        } else if ((*(p + 1) == '-') && (p + 2 < ec)) {
            p += 2;
            if (uchar(*(p - 2)) <= c && c <= uchar(*p))
                return sig;
        } else if (uchar(*p) == c)
            return sig;
    }
    return !sig;
}

static int single_match(int c, const char *p, const char *ep) {
    switch (*p) {
    case '.':
        return 1; /* matches any char */
    case ESC_CHAR:
        return match_class(c, uchar(*(p + 1)));
    case '[':
        return match_bracket_class(c, p, ep - 1);
    default:
        return (uchar(*p) == c);
    }
}

static const char *match(struct match_state *ms, const char *s, const char *p);

static const char *match_balance(struct match_state *ms, const char *s,
                                 const char *p) {
    if (*p == 0 || *(p + 1) == 0)
        logexit("unbalanced pattern");
    if (*s != *p)
        return NULL;
    else {
        int b = *p;
        int e = *(p + 1);
        int cont = 1;
        while (++s < ms->src_end) {
            if (*s == e) {
                if (--cont == 0)
                    return s + 1;
            } else if (*s == b)
                cont++;
        }
    }
    return NULL; /* string ends out of balance */
}

static const char *max_expand(struct match_state *ms, const char *s,
                              const char *p, const char *ep) {
    ptrdiff_t i = 0; /* counts maximum expand for item */
    while ((s + i) < ms->src_end && single_match(uchar(*(s + i)), p, ep))
        i++;
    /* keeps trying to match with the maximum repetitions */
    while (i >= 0) {
        const char *res = match(ms, (s + i), ep + 1);
        if (res)
            return res;
        i--; /* else didn't match; reduce 1 repetition to try again */
    }
    return NULL;
}

static const char *min_expand(struct match_state *ms, const char *s,
                              const char *p, const char *ep) {
    for (;;) {
        const char *res = match(ms, s, ep + 1);
        if (res != NULL)
            return res;
        else if (s < ms->src_end && single_match(uchar(*s), p, ep))
            s++; /* try with one more repetition */
        else
            return NULL;
    }
}

static const char *start_capture(struct match_state *ms, const char *s,
                                 const char *p, int what) {
    const char *res;
    int level = ms->level;
    if (level >= MAX_CAPTURES)
        logexit("too many captures");
    ms->capture[level].init = s;
    ms->capture[level].len = what;
    ms->level = level + 1;
    if ((res = match(ms, s, p)) == NULL) /* match failed? */
        ms->level--; /* undo capture */
    return res;
}

static const char *end_capture(struct match_state *ms, const char *s,
                               const char *p) {
    int l = capture_to_close(ms);
    const char *res;
    ms->capture[l].len = s - ms->capture[l].init; /* close capture */
    if ((res = match(ms, s, p)) == NULL) /* match failed? */
        ms->capture[l].len = CAP_UNFINISHED; /* undo capture */
    return res;
}

static const char *match_capture(struct match_state *ms, const char *s, int l) {
    size_t len;
    l = check_capture(ms, l);
    len = ms->capture[l].len;
    if ((size_t)(ms->src_end - s) >= len &&
        memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    else
        return NULL;
}

static const char *match(struct match_state *ms, const char *s, const char *p) { // 相当于match_here
init: /* using goto's to optimize tail recursion */
    switch (*p) {
    case '(': { /* start capture */
        if (*(p + 1) == ')')
            logexit("empty capture not allowed"); // 禁用position capture
        else
            return start_capture(ms, s, p + 1, CAP_UNFINISHED);
    }
    case ')': { /* end capture */
        return end_capture(ms, s, p + 1);
    }
    case ESC_CHAR: {
        switch (*(p + 1)) {
        case 'b': { /* balanced string? */
            s = match_balance(ms, s, p + 2);
            if (s == NULL)
                return NULL;
            p += 4;
            goto init; /* else return match(ms, s, p+4); */
        }
        case 'f': { /* frontier? */
            const char *ep;
            char previous;
            p += 2;
            if (*p != '[')
                logexit("missing '[' after '%%f' in pattern");
            ep = class_end(ms, p); /* points to what is next */
            previous = (s == ms->src_init) ? '\0' : *(s - 1);
            if (match_bracket_class(uchar(previous), p, ep - 1) ||
                !match_bracket_class(uchar(*s), p, ep - 1))
                return NULL;
            p = ep;
            goto init; /* else return match(ms, s, ep); */
        }
        default: {
            if (isdigit(uchar(*(p + 1)))) { /* capture results (%0-%9)? */
                s = match_capture(ms, s, uchar(*(p + 1)));
                if (s == NULL)
                    return NULL;
                p += 2;
                goto init; /* else return match(ms, s, p+2) */
            }
            goto dflt; /* case default */
        }
        }
    }
    case '\0': { /* end of pattern */
        return s; /* match succeeded */
    }
    case '$': {
        if (*(p + 1) == '\0') /* is the `$' the last char in pattern? */
            return (s == ms->src_end) ? s : NULL; /* check end of string */
        else
            goto dflt;
    }
    default:
    dflt: { /* it is a pattern item */
        const char *ep = class_end(ms, p); /* points to what is next */
        int m = s < ms->src_end && single_match(uchar(*s), p, ep);
        switch (*ep) {
        case '?': { /* optional */
            const char *res;
            if (m && ((res = match(ms, s + 1, ep + 1)) != NULL))
                return res;
            p = ep + 1;
            goto init; /* else return match(ms, s, ep+1); */
        }
        case '*': { /* 0 or more repetitions */
            return max_expand(ms, s, p, ep);
        }
        case '+': { /* 1 or more repetitions */
            return (m ? max_expand(ms, s + 1, p, ep) : NULL);
        }
        case '-': { /* 0 or more repetitions (minimum) */
            return min_expand(ms, s, p, ep);
        }
        default: {
            if (!m)
                return NULL;
            s++;
            p = ep;
            goto init; /* else return match(ms, s+1, ep); */
        }
        }
    }
    }
}

struct var *smatch_s(const char *p, const char *src, size_t srcl) {
    // 参考str_gsub()
    int max_s = srcl + 1;
    int anchor = (*p == '^') ? (p++, 1) : 0;
    int n = 0;
    struct match_state ms;
    ms.src_init = src;
    ms.src_end = src + srcl;
    struct var *result = anew(0);
    while (n < max_s) {
        ms.level = 0;
        const char *e = match(&ms, src, p);
        if (e) {
            // logdebug("\"%s\" \"%s\"", p, src);
            for (int i = 0; i < ms.level; i++) {
                // logdebug("%d: \"%s\" %lld", i, ms.capture[i].init, ms.capture[i].len);
                if (ms.capture[i].len != CAP_UNFINISHED) { // 会不会出现别的异常值？
                    apush(result, snew_s(ms.capture[i].init, ms.capture[i].len));
                }
            }
        }
        if (e && e > src) /* non empty match? */
            src = e; /* skip it */
        else if (src < ms.src_end)
            src++;
        else
            break;
        if (anchor)
            break;
    }
    return result;
}

struct var *smatch(const char *p, const char *srcz) {
    return smatch_s(p, srcz, strlen(srcz));
}
