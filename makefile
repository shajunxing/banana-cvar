.RECIPEPREFIX := $() $()
CC=gcc -s -std=c2x -O3 -Wl,--exclude-all-symbols

all: test_call_stack.exe test_namespace.exe test_try_catch.exe test_var.exe test.exe

clean:
    del /q *.exe *.dll

var.dll: var.h var.c vstring.c varray.c vobject.c vjson.c
    $(CC) -shared -D DLL -D EXPORT -o var.dll var.c vstring.c varray.c vobject.c vjson.c

test_call_stack.exe: test_call_stack.c
    $(CC) -o test_call_stack.exe test_call_stack.c

test_namespace.exe: test_namespace.c
    $(CC) -o test_namespace.exe test_namespace.c

test_try_catch.exe: test_try_catch.c
    $(CC) -o test_try_catch.exe test_try_catch.c

test_var.exe: test_var.c var.dll
    $(CC) -o test_var.exe test_var.c -L. -l:var.dll

test.exe: test.c
    $(CC) -o test.exe test.c -L. -l:var.dll

echo:
    @echo $(OS)
    @echo $(PROCESSOR_ARCHITEW6432)
    @echo $(PROCESSOR_ARCHITECTURE)
