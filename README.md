# A minimal, transparent, robust dynamic extension for C language that supports GC and is compatible with Json data types

Traditional collectors, such as Boehm, are all made on malloc/realloc/free, because C data structures are unpredictable, there is no special markup in memory, it is difficult to distinguish between pointers and data, such as Boehm GC can only do its best to judge. The high-level language defines a complete set of data structures by itself, prohibits users from directly manipulating memory, and the pointers and data are accurately recorded, which can be theoretically ensured, but it seems too cumbersome for the C language. I've always felt that C++/Rust routines are too ugly, because the low-level language niche is to completely expose the underlying data/memory structure, why make so many concepts? The bottom layer is completely opaque, and it is better to use a high-level language.

So my idea is to find the best compromise, follow the most accurate routine, make a fuss on the pointer, more than ten years of experience, Json types are enough, the root pointer of the Mark and Sweep algorithm is the variables created on the stack in the C language, record their state, and then correctly judge whether it is invalid, the leaf pointer is created on the heap, because the data structure is fixed, it can be easily recursively processed. And it's still essentially C code, the underlying structure is fully exposed, and it's just as performant as C, because it's simple to design, and it's easy to optimize and scale.

1. First define the structure var that stores dynamic variables, for the sake of simplicity, the variable type is simple, referring to the Json standard, null/boolean/number/array/object is sufficient, the use of dynamic variables is to use pointers to allocate memory on the heap, and all dynamic variables are hosted on the pvarroot globally linked list.
2. Then define the structure ref that stores the stack variable information, the stack variable is the var * variable defined in the C source code, called the reference, and the address of the reference and the address pointing to the dynamic variable are registered through the vdeclare/vassign macro, and registered on the global linked list prefroot.
3. Finally, and most importantly, in the garbage collection phase, if the address that the reference points to is different from the address that was previously registered, it means that the reference is invalid (for example, the stack value has changed or been modified after the function call is returned), perform a delete operation, mark the dynamic variable (array/object needs to be recursive) pointed to by the valid reference as in use, and then delete the unused dynamic variable.
4. Expand the function as needed, consider designing a set of extended dynamic syntax, and write a translator to translate it into C code.
5. There are very few global variables, if you need multi-threading support, you can wrap them into the struct and pass them as function parameters, but I don't think it's necessary, and each thread has one set, and they can't interact with each other, how awkward to look at.

In order to follow the usage habits of most dynamic languages, the following design guidelines are specified:

1. Be sure to use the macro vdeclare/vassign to create dynamic variables, if not necessary, do not directly manipulate the var * pointer, unless you know what you are doing; the free var * pointer does not call refer to register as a reference, which is equivalent to a weak reference, and will be deleted during garbage collection; the unassigned var * pointer (pointing to NULL) will exit abnormally in most functions.
2. Assign a variable to B variable, the actual pass is the address, in order to save memory, null/true/false are global constants, all the original values of number/string are to create new variables, not to modify the original variables, array/object has another set of functions for addition, deletion and modification.
3. For exception handling, all unrecoverable errors are executed with the exitif macro to exit the process.
4. I also forgot to use the latest standard of the C language to what features, anyway, the latest version of GCC is compiled and passed, including the MinGW makefile, other operating systems modify their own.
5. It's not a toy, my idea is to replace something complicated like C++/Rust that has gone astray, and I haven't found any serious problems such as memory leaks during my testing anyway.
6. Function naming convention: optional abbreviation + complete word + optional suffix, the meaning of the abbreviation is as follows, there is only one suffix currently used, _s represents the string parameter with a length parameter of the safe function, C language string literal can safely use the function without _s, and it is recommended to use the _s in other cases.

|Abbreviations|descriptions|
|-|-|
|v|Dynamically typed variables|
|r|reference|
|z|zero, zilch, zip, null, nil, nought, nothing|
|b|boolean|
|n|number|
|s|string|
|a|array|
|o|object|
|w|world, whole, which means the whole and the whole|
|sb|stringbuffer helper|

Here's a list of functions/macros, along with the syntax of the corresponding common dynamic languages (you can understand them at a glance), and examples:

|Functions/Macros|Dynamic Syntax|Descriptions/Examples|
|-|-|-|
|dump()||Print all constants, variables, and references|
|void gc()||Garbage Collection|
|vassign(a, b)|a = b||
|vdeclare(a, b)|var a = b||
|struct var *znew()||Returns an address that points to a null constant |
|zdeclare(a)|var a = null||
|struct var *bnew(bool b)||Returns an address that points to the true/false constant |
|bdeclare(a, b)|var a = true|bdeclare(a, true)|
|bool bvalue(struct var *pv)||Returns the original value of the boolean variable|
|struct var *nnew(double n)||Create the number variable|
|ndeclare(a, b)|var a = 3.14|ndeclare(a, 3.14)|
|double nvalue(struct var *pv)||Returns the original value of the number variable|
|struct var *snew_s(const char *s, size_t slen)<br>var *snew(const char *sz)||Create a string variable |
|sdeclare(a, b)|var a = "hello"|sdeclare(a, "hello")|
|char *svalue(struct var *pv)||Returns the original value of the string variable (pointing to an array of characters ending in 0)|
|size_t slength(struct var *pv)|a.length()|returns string length|
|struct var *sconcat(size_t num, ...)|var a = "hi"<br>var b = "all"<br>var c = a + b|sdeclare(a, "hi")<br>sdeclare(b, "all")<br>vdeclare(c, sconcat(2, a, b))|
|struct var *anew(size_t num, ...)|var a = [null, true, 3.14, "hello"]|Create an array variable with 0 or more members<br>, vdeclare(a, anew(4, znew(), bnew(true), nnew(3.14), snew("hello")))|
|adeclare(a)|var a = []||
|void aclear(struct var *pv)|v.clear()|empty array|
|size_t alength(struct var *pv)|v.length()|returns array length|
|void apush(struct var *pv, struct var *pval)|v.push(val)|insertion at the end of the array, length +1|
|struct var *apop(struct var *pv)|v.pop()|array pops up at the end of the array, length -1|
|void aput(struct var *pv, size_t idx, struct var *pval)|v[idx] = val|array specifies that the subscript is written to replace the original value, and the program exits if the subscript is out of bounds |
|struct var *aget(struct var *pv, size_t idx)|v[idx]|takes the value of the array to specify the subscript, and the program exits if the subscript is out of bounds|
|asort(....)||Array sorting, not implemented|
|struct var *onew()||Create an empty object|
|odeclare(a)|var a = {}||
|void oclear(struct var *pv)|v.clear()|empty object|
|size_t olength(struct var *pv)|v.length()|returns the number of elements of the object|
|void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval)<br>void oput(struct var *pv, const char *key, struct var *pval)|v[key] = val|write key-value pair|
|struct var *oget_s(struct var *pv, const char *key, size_t klen)<br>struct var *oget(struct var *pv, const char *key)|v[key]|read the value corresponding to the key, note that NULL may be returned, and the program must make a judgment|
|struct var *vtojson(struct var *pv)||jsonize arbitrary variables, and the result is returned as a string variable, note: nesting dolls are prohibited, and nested loops are ignored|
|tojson(pv)|print([null, [3.140000, "hi"], false])|Returns an array of C characters jsonized by the variable, which is used for functions such as printf<br>printf(tojson(anew(3, znew(), anew(2, nnew(3.14), snew("hi")), bnew(false))))|
|fromjson(...)||Parsing JSON strings, building variables, not implemented.|

For more sample code, see test_var.c
