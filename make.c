/*
Copyright 2024-2025 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../banana-make/make.h"

#define bin_dir "bin" pathsep
#define build_dir "build" pathsep
#define src_dir "src" pathsep
#define banana_script_src_dir ".." pathsep "banana-script" pathsep "src" pathsep
#define js_common_h banana_script_src_dir "js-common.h"
#define js_common_c banana_script_src_dir "js-common.c"
#define js_common_obj build_dir "js_common" objext
#define js_data_h banana_script_src_dir "js-data.h"
#define js_data_c banana_script_src_dir "js-data.c"
#define js_data_obj build_dir "js_data" objext
#define var_h src_dir "var.h"
#define var_c src_dir "var.c"
#define var_obj build_dir "var" objext
#define example_c src_dir "example.c"
#define example_obj build_dir "example" objext
#define example_exe bin_dir "example" exeext
#ifdef _MSC_VER
    #define cc "cl /nologo /c /W3 /MD /Zp /utf-8 /std:clatest /Fo"
    #define compile_js_common cc js_common_obj " " js_common_c
    #define compile_js_data cc js_data_obj " " js_data_c
    #define compile_var cc var_obj " " var_c
    #define compile_example cc example_obj " " example_c
#endif

void build() {
    mkdir(bin_dir);
    mkdir(build_dir);
    if (mtime(js_common_obj) < mtime(js_common_h, js_common_c)) {
        async(compile_js_common);
    }
    if (mtime(js_data_obj) < mtime(js_data_h, js_data_c, js_common_obj)) {
        async(compile_js_data);
    }
    if (mtime(var_obj) < mtime(var_h, var_c, js_data_obj js_common_obj)) {
        async(compile_var);
    }
    if (mtime(example_obj) < mtime(example_c, var_obj, js_data_obj js_common_obj)) {
        async(compile_example);
    }
    await();
    if (mtime(example_exe) < mtime(example_obj, var_obj, js_data_obj, js_common_obj)) {
        run("link /nologo /incremental:no /out:" example_exe " " example_obj " " var_obj " " js_data_obj " " js_common_obj);
    }
}

void cleanup(const char *dir, const char *base, const char *ext) {
    if (base) {
        char *file_name = concat(dir, base, ext);
        remove(file_name);
        free(file_name);
    } else {
        listdir(dir, cleanup);
        rmdir(dir);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        build();
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        if (equals(argv[1], "clean")) {
            listdir(bin_dir, cleanup);
            listdir(build_dir, cleanup);
            return EXIT_SUCCESS;
        } else if (equals(argv[1], "-h", "--help")) {
            ;
        } else {
            printf("Invalid target: %s\n", argv[1]);
        }
    } else {
        printf("Too many arguments\n");
    }
    printf("Usage: %s [clean|-h|--help], default is debug\n", argv[0]);
    return EXIT_FAILURE;
}
