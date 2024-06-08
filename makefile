.RECIPEPREFIX := $() $()

# 区分debug/release模式，默认debug，可打印堆栈追踪
# 参见 https://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds
# 注意-lbacktrace要放在最后面
# 参考 https://stackoverflow.com/questions/11893996/why-does-the-order-of-l-option-in-gcc-matter
debug: CC = gcc -g -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols -static -static-libgcc
debug: LDFLAGS = -lbacktrace
debug: all

release: CC = gcc -s -O3 -Wall -std=gnu2x -Wl,--exclude-all-symbols -static -static-libgcc -D NDEBUG
release: LDFLAGS = 
release: all

all: var.dll example.exe

clean:
    rm -f *.exe *.dll

SOURCES = var.c buffer.c string.c array.c object.c json.c unicode.c

var.dll: var.h $(SOURCES)
    $(CC) -shared -D DLL -D EXPORT -o var.dll $(SOURCES) $(LDFLAGS)

example.exe: example.c var.dll
    $(CC) -o example.exe example.c -l:var.dll

