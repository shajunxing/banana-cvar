/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "var.h"

static void _pause() {
    printf("Press return to continue ...");
    getchar();
}

static void _create_values() {
    var(a, null());
    var(b, boolean(true));
    var(c, boolean(false));
    var(d, number(3.14));
    var(e, scripture("This is a constant string"));
    var(f, string("This is a dynamic string"));
    print(a, b, c, d, e, f);
    gc();
    print(a, b, c, d, e, f);
}

static void _loop_without_gc() {
    printf("Open task manager to see memory consumption grows\n");
    _pause();
    for (;;) {
        var(s, string("This is a dynamic string"));
    }
}

static void _loop_with_gc() {
    printf("Open task manager to see memory consumption keeps unchanged\n");
    _pause();
    for (;;) {
        var(s, string("This is a dynamic string"));
        gc();
    }
}

static void _exception_handling() {
    try(try(throw(scripture("Boom!")), ex, print(ex); throw(scripture("Bars!"));), ex, print(ex));
    printf("%zu\n", error_stack.length);
    try(
        try({
            var(a, number(3.141592654));
            var(b, number(2.718281829));
            var(c, string("Hello,"));
            var(d, string("World!"));
            print(a, b, add(a, b), c, d, add(c, d));
            print(add(a, c)); }, ex, throw(ex)), ex, print(ex));
    gc();
}

#define example_function_list \
    X(_create_values) \
    X(_loop_without_gc) \
    X(_loop_with_gc) \
    X(_exception_handling)

#define X(name) #name,
static const char *example_function_names[] = {example_function_list};
#undef X

#define X(name) name,
static const void (*example_functions[])() = {example_function_list};
#undef X

static void _read_line(char **base, size_t *length, size_t *capacity) {
    enforce(*base == NULL);
    enforce(*length == 0);
    enforce(*capacity == 0);
    for (;;) {
        int ch = getchar();
        if (ch == EOF || ch == '\n') {
            break;
        }
        string_buffer_append_ch(*base, *length, *capacity, ch);
    }
}

int main(int argc, char *argv[]) {
    int id;
    for (id = 0; id < countof(example_function_names); id++) {
        printf("%4d. %-34s", id, example_function_names[id]);
        if (id % 2 == 1) {
            printf("\n");
        }
    }
    if (id % 2 == 1) {
        printf("\n");
    }
    for (;;) {
        printf("Enter your choice (0-%zu): ", countof(example_functions) - 1);
        char *line = NULL;
        size_t line_length = 0;
        size_t line_capacity = 0;
        _read_line(&line, &line_length, &line_capacity);
        int selected_id = atoi(line);
        buffer_free(line, line_length, line_capacity);
        if (selected_id >= 0 && selected_id < countof(example_functions)) {
            example_functions[selected_id]();
            break;
        }
    }
    return EXIT_SUCCESS;
}