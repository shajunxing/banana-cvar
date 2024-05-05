.RECIPEPREFIX := $() $()

# 区分debug/release模式，默认debug，可打印堆栈追踪
# 参见 https://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds
debug: CC = gcc -g -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols -DDEBUG
debug: all

release: CC = gcc -s -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols
release: all

all: var.dll example.exe

clean:
    rm -f *.exe *.dll

# 注意-lbacktrace要放在最后面
# 参考 https://stackoverflow.com/questions/11893996/why-does-the-order-of-l-option-in-gcc-matter
var.dll: var.h var.c vbuffer.c vstring.c varray.c vobject.c vjson.c
    $(CC) -shared -D DLL -D EXPORT -o var.dll var.c vbuffer.c vstring.c varray.c vobject.c vjson.c -lbacktrace

example.exe: example.c
    $(CC) -o example.exe example.c -l:var.dll

echo:
    @echo $(OS)
    @echo $(PROCESSOR_ARCHITEW6432)
    @echo $(PROCESSOR_ARCHITECTURE)
