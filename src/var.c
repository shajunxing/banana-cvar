/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdarg.h>
#include <stdlib.h>
#include "var.h"

void _print(size_t nargs, ...) {
    va_list args;
    va_start(args, nargs);
    for (size_t i = 0; i < nargs; i++) {
        struct js_value v = va_arg(args, struct js_value);
        js_value_print(&v);
        printf(" ");
    }
    printf("\n");
    va_end(args);
}

struct heap default_heap = {0};
struct record default_record = {0};

struct js_value _string(struct heap *hl, struct record *rl, const char *s, size_t slen) {
    struct js_value ret = js_string(&(hl->base), &(hl->length), &(hl->capacity), s, slen);
    buffer_push(rl->base, rl->length, rl->capacity, ((struct variable){.value = ret, .address = &ret}));
    return ret;
}

void _gc(struct heap *hl, struct record *rl) {
    struct record new_rl = {0};
    buffer_for_each(rl->base, rl->length, rl->capacity, i, r, {
        if (memcmp(&(r->value), r->address, sizeof(struct js_value)) == 0) { // stack not changed, means in use
            js_mark(&(r->value));
            buffer_push(new_rl.base, new_rl.length, new_rl.capacity, *r);
        }
    });
    buffer_free(rl->base, rl->length, rl->capacity);
    *rl = new_rl;
    js_sweep(&(hl->base), &(hl->length), &(hl->capacity));
}

struct error_stack default_error_stack = {0};

struct js_value _add(struct heap *hl, struct record *rl, struct js_value lhs, struct js_value rhs) {
    if (lhs.type == vt_number && rhs.type == vt_number) {
        return js_number(lhs.number + rhs.number);
    } else if (js_is_string(&lhs) && js_is_string(&rhs)) {
        struct js_value ret = _string(hl, rl, js_string_base(&lhs), js_string_length(&lhs));
        string_buffer_append(ret.managed->string.base, ret.managed->string.length, ret.managed->string.capacity, js_string_base(&rhs), js_string_length(&rhs));
        return ret;
    } else {
        throw(js_scripture_sz("Add operand must be number or string"));
    }
}