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

static struct js_heap _heap = {0};
struct record record = {0};

struct js_value string(const char *s) {
    return js_string_sz(&_heap, s);
}

void gc() {
    struct record new_record = {0};
    buffer_for_each(record.base, record.length, record.capacity, i, r, {
        if (memcmp(&(r->value), r->address, sizeof(struct js_value)) == 0) { // stack not changed, means in use
            js_mark(&(r->value));
            buffer_push(new_record.base, new_record.length, new_record.capacity, *r);
        }
    });
    buffer_free(record.base, record.length, record.capacity);
    record = new_record;
    js_sweep(&_heap);
}

struct error_stack error_stack = {0};

#define _proxy_js_result(__arg_expression)              \
    do {                                                \
        struct js_result __result = (__arg_expression); \
        if (__result.success) {                         \
            return __result.value;                      \
        } else {                                        \
            throw(__result.value);                      \
        }                                               \
    } while (0)

struct js_value add(struct js_value lhs, struct js_value rhs) {
    _proxy_js_result(js_add(&_heap, &lhs, &rhs));
}