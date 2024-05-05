# A simplified, fast, transparent, and robust dynamic extension of C language, supports garbage collection, and is compatible with Json data types

[English Readme](README.md) | [中文自述文件](自述文件.md) | [English Changelog](CHANGELOG.md) | [中文更改日志](更改日志.md)

[Project address](https://github.com/shajunxing/banana-cvar)

## Design ideas

Traditional collectors, such as Boehm, are all made on malloc/realloc/free, because C data structures are unpredictable, there is no special markup in memory, it is difficult to distinguish between pointers and data, such as Boehm GC can only do its best to judge. The high-level language defines a complete set of data structures by itself, prohibits users from directly manipulating memory, and the pointers and data are accurately recorded, which can be theoretically ensured, but it seems too cumbersome for the C language. I've always felt that C++/Rust routines are too ugly, because the low-level language niche is to completely expose the underlying data/memory structure, why make so many concepts? The bottom layer is completely opaque, and it is better to use a high-level language.

So my idea is to find the best compromise, follow the most accurate routine, make a fuss on the pointer, more than ten years of experience, Json types are enough, the root pointer of the marker removal algorithm is the variables created on the stack in the C language, record their state, and then correctly judge whether it is invalid, the leaf pointer is created on the heap, because the data structure is fixed, it can be easily recursively processed. And it's still essentially C code, the underlying structure is fully exposed, and it's just as performant as C, because it's simple to design, and it's easy to optimize and scale.

I developed in Windows/MSYS2/MinGW64 environment, makefile is suitable for MSYS2, using the latest C standard and GCC extension syntax, for exception handling, all unrecoverable errors are forcibly exited from the process, stack trace uses libbacktrace library.

## Variables and references

Use the xnew() series of functions to create dynamic variables, x is the prefix, which represents the abbreviation of the data type, as shown in the following table, all function names are in the style of "optional prefix + name + optional suffix", the suffix is currently only used one, _s, it represents the safe version of the function, such as the function containing the character array parameters, if it is a string literal, you can use the unsafe function, in other cases it is recommended to use the safe function, that is, the length of the string must be specified.

|prefix|description|
|-|-|
|v|dynamically typed variables|
|r|reference|
|z|zero, zilch, zip, which represent null, nil, nought, nothing|
|b|boolean|
|n|number|
|s|string|
|a|array|
|o|object|
|w|world, whole, which means the whole and the whole |

Dynamic variables are corresponding to struct var, which are operated by pointers, and the types are equivalent to the Json standard, which are null, boolean, number, array, and object, and null, true, and false are global constants in order to save memory. Dynamic variables created by the xnew() series of functions are hosted on the globally linked list pvarroot. The functions and macros listed in the following table are all functions with arguments and return value types, and vice versa. The so-called dynamic syntax is to give an intuitive feeling with a JavaScript-like syntax, and in the future, it may be extended in this way and a converter can be written to convert it to C code, so that it can perfectly replace embedded dynamic languages such as Lua.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|struct var *znew()||Create a null value|
|struct var *bnew(bool b)||Create a Boolean value|
|struct var *nnew(double n)||Create a numeric value|
|struct var *snew_s(const char *s, size_t slen)<br>var *snew(const char *sz)||Create a string|
|struct var *anew(size_t num, ...)|var a = [null, true, 3.14, "hello"]|Create an array of 0 or more <br>membersvdeclare(a, anew(4, znew(), bnew(true), nnew(3.14), snew("hello")))|
|struct var *onew()||Create an empty object|

Dynamic variables that do not have a reference to direct or indirect pointers will be cleaned up when the gc() function is executed. References are, first, named variables of type struct var*, existing on the stack, and second, they must be registered on the globally linked list prefroot, after which the mark-up algorithm of the gc() function can correctly identify their lifetimes. If there are no special circumstances, be sure to use the following macro to create a variable and re-assign the variable, remind again, the unregistered variable is equivalent to a weak reference, and will be deleted when garbage collection; in addition, the macro will assign a value to the reference, which is necessary, because most functions do not allow the NULL value as a parameter, which will trigger an abnormal exit; assign a variable to b variable, and actually pass the address; all assignments of the original number/string value are to create a new variable, not modify the original variable;array/ object also has a set of functions for adding, deleting, and modifying.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|vdeclare(a, b)|var a = b|declare macros are assigned to declare variables at the same time
|zdeclare(a)|var a = null<br>, i.e. var a|null|
|bdeclare(a, b)|var a = true|, assign a boolean value <br>bdeclare(a, true)|
|ndeclare(a, b)|var a = 3.14|assign the value <br>ndeclare(a, 3.14)|
|sdeclare(a, b)|var a = "hello"|assign the string <br>sdeclare(a, "hello")|
|adeclare(a)|var a = []|null array<br>|
|odeclare(a)|var a = {}|nullify object<br>|
|vassign(a, b)|a = b|assign a new value to an existing variable
|zassign(a)|a = null|null|null|null|
|bassign(a, b)|a = true|, assign the boolean value <br>bassign(a, true)|
|nassign(a, b)|a = 3.14|<br>nassign(a, 3.14)|
|sassign(a, b)|a = "hello"|assign the string <br>sassign(a, "hello")|
|aassign(a)|a = []|nullify array<br>|
|oassign(a)|a = {}|nullify object<br>|

## Generic functions

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|void gc()||Garbage Collection|
|enum vtype vtype(struct var *pv)||Returns the variable type in the enum format, with the following values: vtnull, vtboolean, vtnumber, vtstring, vtarray, and vtobject|
|void stacktrace()||Print stack trace information|
|void vdump(const char *prefix, const struct var *pv)||Print debug information for a single dynamic variable|
|void wdump(const char *suffix)<br>dump()||Prints all constants, variables, and references|

## Boolean

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|bool bvalue(struct var *pv)||Returns the original value of the boolean variable|

## Numeric

Values are numbers, and they do not distinguish between integers and floating-point numbers, and can be replaced with other distinguishable or high-precision numerical computing libraries if necessary, but do not distinguish at the API level.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|double nvalue(struct var *pv)||Returns the original value of the number variable|

## Strings

All of the internal buffer storage structures I've designed, including strings, arrays, and objects, are uniform and optimized, but I haven't thought of a more appropriate API interface for string connections just yet.

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|const char *svalue(struct var *pv)||Returns the original value of the string variable (pointing to an array of characters ending in 0)|
|size_t slength(struct var *pv)|a.length()|returns string length|
|struct var *sconcat(size_t num, ...)|var a = "hi" <br>var b = "all" <br>var c = a + b|concatenate one or more <br>strings sdeclare(a, "hi")<br>sdeclare(b, "all")<br>vdeclare(c, sconcat(2, a, b))|
|struct var *sformat(const char *fmt, ...)||Return the formatted string, with the same parameters as printf|

## Arrays

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|void aclear(struct var *pv)|v.clear()|empty array|
|size_t alength(struct var *pv)|v.length()|returns array length|
|void apush(struct var *pv, struct var *pval)|v.push(val)|insertion at the end of the array, length +1|
|struct var *apop(struct var *pv)|v.pop()|array pops up at the end of the array, length -1|
|void aput(struct var *pv, size_t idx, struct var *pval)|v[idx] = val|array specifies that the subscript is written to replace the original value, and the program exits | if the subscript is out of bounds
|struct var *aget(struct var *pv, size_t idx)|v[idx]|takes the value of the array to specify the subscript, and the program exits if the subscript is out of bounds|
|void asort(struct var *pv, int (*comp)(const struct var *, const struct var *))|v.sort()|array sorting, comp is a user-specified sorting function, the return value convention is the same as qsort, if it is NULL, then call the default sorting function, the rules are: 1. Different types, according to null, boolean, number, 2. The string is called memcmp with the minimum length +1 before true, that is, 0 at the end participates in the comparison, and the array and object only compare the number of elements |
|void aforeach(struct var *arr, void (*cb)(size_t i, struct var *v))|for (i, v in arr) {...}|Sequentially iterate through all indexes and values of the array|

## Objects

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|void oclear(struct var *pv)|v.clear()|empty object|
|size_t olength(struct var *pv)|v.length()|returns the number of elements of the object|
|void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval)<br>void oput(struct var *pv, const char *key, struct var *pval)|v[key] = val|write key-value pair|
|struct var *oget_s(struct var *pv, const char *key, size_t klen)<br>struct var *oget(struct var *pv, const char *key)|v[key]|read the value corresponding to the key, note that NULL may be returned, and the program must make a judgment|
|void oforeach(struct var *obj, void (*cb)(const char *k, size_t klen, struct var *v))|for (k, klen, v in obj) {...}|Iterate through all keys and values of the object|

## Json

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|struct var *vtojson(struct var *pv)||jsonize arbitrary variables, and the result is returned as a string variable, note: nesting dolls are prohibited, and nested loops are ignored|
|tojson(pv)|print([null, [3.140000, "hi"], false])|Returns an array of C characters jsonized by the variable, which is used for functions such as printf<br>printf(tojson(anew(3, znew(), anew(2, nnew(3.14), snew("hi")), bnew(false))))|
|struct var *vfromjson_s(const char *jsonstr, size_t jsonslen)<br>struct var *vfromjson(const char *jsonstr)||Parsing JSON strings, this function will skip all non-essential characters, so it has a strong error correction ability, in addition, UTF-16 escape is not supported, because it has long been abandoned, JSON strings themselves are forced UTF-8 encoding|

## Code sample

For more sample code, see example.c




