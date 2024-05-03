.RECIPEPREFIX := $() $()

# 区分debug/release模式，默认debug，可打印堆栈追踪
# 参见 https://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds
debug: CC = gcc -g -O1 -Wall -std=gnu2x -Wl,--exclude-all-symbols -DDEBUG
debug: all

release: CC = gcc -s -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols
release: all

all: test_call_stack.exe test_namespace.exe test_try_catch.exe test_var.exe test.exe test_allocator.exe

clean:
    rm -f *.exe *.dll

# 注意-lbacktrace要放在最后面
# 参考 https://stackoverflow.com/questions/11893996/why-does-the-order-of-l-option-in-gcc-matter
var.dll: var.h var.c vbuffer.c vstring.c varray.c vobject.c vjson.c
    $(CC) -shared -D DLL -D EXPORT -o var.dll var.c vbuffer.c vstring.c varray.c vobject.c vjson.c -lbacktrace

test_call_stack.exe: test_call_stack.c
    $(CC) -o test_call_stack.exe test_call_stack.c

test_namespace.exe: test_namespace.c
    $(CC) -o test_namespace.exe test_namespace.c

test_try_catch.exe: test_try_catch.c
    $(CC) -o test_try_catch.exe test_try_catch.c

test_var.exe: test_var.c var.dll
    $(CC) -o test_var.exe test_var.c -L. -l:var.dll

test.exe: test.c var.dll
    $(CC) -o test.exe test.c -L. -l:var.dll

test_allocator.exe: test_allocator.c var.dll
    $(CC) -o test_allocator.exe test_allocator.c -L. -l:var.dll

echo:
    @echo $(OS)
    @echo $(PROCESSOR_ARCHITEW6432)
    @echo $(PROCESSOR_ARCHITECTURE)
