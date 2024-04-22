# A simplified, fast, transparent, and robust dynamic extension of C language, supports garbage collection, and is compatible with Json data types

Project address: <https://github.com/shajunxing/banana-cvar>

## Design ideas

Traditional collectors, such as Boehm, are all made on malloc/realloc/free, because C data structures are unpredictable, there is no special markup in memory, it is difficult to distinguish between pointers and data, such as Boehm GC can only do its best to judge. The high-level language defines a complete set of data structures by itself, prohibits users from directly manipulating memory, and the pointers and data are accurately recorded, which can be theoretically ensured, but it seems too cumbersome for the C language. I've always felt that C++/Rust routines are too ugly, because the low-level language niche is to completely expose the underlying data/memory structure, why make so many concepts? The bottom layer is completely opaque, and it is better to use a high-level language.

So my idea is to find the best compromise, follow the most accurate routine, make a fuss on the pointer, more than ten years of experience, Json types are enough, the root pointer of the marker removal algorithm is the variables created on the stack in the C language, record their state, and then correctly judge whether it is invalid, the leaf pointer is created on the heap, because the data structure is fixed, it can be easily recursively processed. And it's still essentially C code, the underlying structure is fully exposed, and it's just as performant as C, because it's simple to design, and it's easy to optimize and scale.

The corresponding C structure of dynamic variables is struct var, the type refers to the Json standard, which is null/boolean/number/array/object, and all dynamic variables are hosted on the globally linked list pvarroot. The operation of dynamic variables is in the form of pointers, that is, struct var *, and the pointers themselves are also variables, which are divided into two categories: one is stack variables, which are directly named in the source code, called references, and are also managed, using struct ref to record information and hang on the global linked list prefroot, and the other is heap variables, which are dynamically created and anonymous, such as pointers to child variables in array/object. The most important thing in the garbage collection algorithm is the principle of judging the invalidity of the reference, that is, the address that the reference now points to is different from the address pointed to by the previous registration, such as the stack value changed or was artificially modified after the function call was returned.

Developed in the GCC environment, using the latest C standard and GCC extension syntax, for exception handling, all unrecoverable errors are executed to exit the process, and the stack trace uses the libbacktrace library. I'm developing in a Windows environment, and the makefile is for MSYS2.

## Function naming conventions

Optional prefix + name + optional suffix, the meaning of the prefix is as follows.

|prefix|description|
|-|-|
Dynamically typed variables|v|
|r|citation|
|z|zero, zilch, zip, which represent null, nil, nought, nothing|
|b|boolean|
|n|number|
|s|string|
|a|array|
|o|object|
|w|world, whole, which means the whole and the whole |
|sb|stringbufferhelper function|

The suffix _s indicates a secure version of the function, such as a function containing an array of character parameters, if it is a string literal, it can be used as an unsafe function, and in other cases it is recommended to use a safe function, that is, the length of the string must be specified.

## Declarations and assignments of variables

If there are no special circumstances, be sure to use the provided macro to create variables, the function of the macro is to call the refer function to register the pointer variable, the unregistered is equivalent to a weak reference, which will be deleted when garbage collection; in addition, the macro will assign a value to the reference, which is necessary, because most functions do not allow the NULL value as a parameter, which will trigger an abnormal exit; assign a variable to b variable, the actual pass is the address; all assigned number/ The original value of string is to create a new variable instead of modifying the original variable, and array/object has a set of functions to add, delete and modify.

The following table lists functions and macros, both with arguments and return value types are functions, and vice versa are macros. The so-called dynamic syntax is to give an intuitive feeling with a JavaScript-like syntax, and in the future, it may be extended in this way and a converter can be written to convert it to C code, so that it can perfectly replace embedded dynamic languages such as Lua.

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
|struct var *znew()||Create a null value. The new series of functions to create new dynamic variables, note that this kind of function is generally not used directly, but is included in the declare and assign series of macros, in order to save memory, null/true/false are all global constants|
|struct var *bnew(bool b)||Create a Boolean value|
|struct var *nnew(double n)||Create a numeric value|
|struct var *snew_s(const char *s, size_t slen)<br>var *snew(const char *sz)||Create a string|
|struct var *anew(size_t num, ...)|var a = [null, true, 3.14, "hello"]|Create an array of 0 or more <br>membersvdeclare(a, anew(4, znew(), bnew(true), nnew(3.14), snew("hello")))|
|struct var *onew()||Create an empty object|

## Generic and helper functions

Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|lambda(ret, args, body)||Use GCC's compound statements and nested function syntax to define the anonymous function <br>lambda(int, (int a, int b), {return a + b;})|
|void stacktrace()||Print stack trace information|
|void alloc_s(void **pp, size_t oldnum, size_t newnum, size_t sz)||Secure Request Memory|
|void free_s(void **pp)||Securely free memory|
|sbdeclare(var_name)||Declare stringbuffer and encapsulate strings
|void sbclear(struct stringbuffer *psb)||After the stringbuffer is used, you need to call this function to release memory |
|void sbappend_s(struct stringbuffer *psb, const char *s, size_t slen)<br>void sbappend(struct stringbuffer *psb, const char *sz)||Append the specified string to the end of the |
|void sbdump(struct stringbuffer *psb)||Print the debug information of the stringbuffer|
|void vdump(const char *prefix, const struct var *pv)||Print debug information for a single dynamic variable|
|void wdump(const char *suffix)<br>dump()||Prints all constants, variables, and references|
|void gc()||Garbage Collection|
|void refer(struct var **ppv, char *descr, struct var *pval)||To register or update reference information, this function is generally not called directly, but is included in the declare and assign series of macros
|enum vtype vtype(struct var *pv)||Returns the variable type in the enum format, with the following values: vtnull, vtboolean, vtnumber, vtstring, vtarray, and vtobject|

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
|char *svalue(struct var *pv)||Returns the original value of the string variable (pointing to an array of characters ending in 0)|
|size_t slength(struct var *pv)|a.length()|returns string length|
|struct var *sconcat(size_t num, ...)|var a = "hi" <br>var b = "all" <br>var c = a + b|concatenate one or more <br>strings sdeclare(a, "hi")<br>sdeclare(b, "all")<br>vdeclare(c, sconcat(2, a, b))|

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
|fromjson(...)||Parsing JSON strings, building variables, not implemented.|

## Code sample

For more sample code, see test_var.c




