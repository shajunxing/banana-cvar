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

struct managed_list default_managed_list = {0};
struct reference_list default_reference_list = {0};

struct js_value _string(struct managed_list *ml, struct reference_list *rl, const char *s, size_t slen) {
    struct js_value ret = js_string(&(ml->base), &(ml->length), &(ml->capacity), s, slen);
    buffer_push(rl->base, rl->length, rl->capacity, ((struct reference){.value = ret, .address = &ret}));
    return ret;
}

void _gc(struct managed_list *ml, struct reference_list *rl) {
    struct reference_list new_rl = {0};
    buffer_for_each(rl->base, rl->length, rl->capacity, i, r, {
        if (memcmp(&(r->value), r->address, sizeof(struct js_value)) == 0) { // stack not changed, means in use
            js_mark(&(r->value));
            buffer_push(new_rl.base, new_rl.length, new_rl.capacity, *r);
        }
    });
    buffer_free(rl->base, rl->length, rl->capacity);
    *rl = new_rl;
    js_sweep(&(ml->base), &(ml->length), &(ml->capacity));
}