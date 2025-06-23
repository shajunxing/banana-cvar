/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef VAR_H
#define VAR_H

#include "../../banana-script/src/js-data.h"

typedef struct js_value var;
#define null() js_null()
#define boolean(__arg_b) js_boolean(__arg_b)
#define number(__arg_n) js_number((double)__arg_n)
#define scripture(__arg_s) js_scripture_sz(__arg_s)

#ifndef numargs
    // https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
    // in msvc, works fine even with old version
    // in gcc, only works with default or -std=gnu2x, -std=c?? will treat zero parameter as 1, maybe ## is only recognized by gnu extension
    #if defined(__GNUC__) && defined(__STRICT_ANSI__)
        #error numargs() only works with gnu extension enabled
    #endif
    #define _numargs_call(__arg_0, __arg_1) __arg_0 __arg_1
    #define _numargs_select(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, __arg_0, ...) __arg_0
    #define numargs(...) _numargs_call(_numargs_select, (_, ##__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

extern void _print(size_t, ...);
#define print(...) _print(numargs(__VA_ARGS__), ##__VA_ARGS__)

struct managed_list {
    struct js_managed_value **base;
    size_t length;
    size_t capacity;
};

struct reference {
    struct js_value value;
    struct js_value *address;
};

struct reference_list {
    struct reference *base;
    size_t length;
    size_t capacity;
};

extern struct managed_list default_managed_list;
extern struct reference_list default_reference_list;

extern struct js_value _string(struct managed_list *, struct reference_list *, const char *, size_t);
#define string(__arg_s) _string(&default_managed_list, &default_reference_list, __arg_s, strlen(__arg_s))

extern void _gc(struct managed_list *, struct reference_list *);
#define gc() _gc(&default_managed_list, &default_reference_list)

#endif