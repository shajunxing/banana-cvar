我用过很多高级语言，喜欢简单的东西，讨厌C++，一直在想C语言能不能用最简洁的手段扩充动态语言特性，并且支持垃圾回收呢？偶然迸发出灵感，网上查了查没有人做过，那么开始动手吧。

思路很简单：

1. 首先定义存储动态变量的结构体var，变量类型简单起见，参照Json标准，null/boolean/number/array/object足够用了，动态变量的使用均采用指针指向堆上分配内存的方式，所有动态变量都托管在pvarroot全局链表上。
2. 然后定义存储栈变量信息的结构体ref，栈变量就是c源代码里定义的var *变量，称之为引用，通过vdeclare/vassign宏登记引用的地址以及指向动态变量的地址，登记在全局链表prefroot上。
3. 最后最关键的，在垃圾收集阶段，如果引用现在指向的地址与先前登记指向的地址不同，就意味着该引用已失效（比如函数调用返回后栈值变化了、或者人为修改了），执行删除操作，把有效引用指向的动态变量（array/object需要递归）标记为正在使用中，然后对未使用的动态变量执行删除操作。
4. 后续按需扩充功能，考虑设计一套扩展的动态语法，并写个翻译器翻译成c代码。
5. 全局变量很少，如果需要多线程支持，可以包进结构体里并作为函数参数传递，但我认为没有必要，且每个线程一套，又不能相互交互，怎么看怎么别扭。

为了遵循大部分动态语言的使用习惯，规定以下的设计准则：

1. 务必用宏vdeclare/vassign/var创建动态变量，如无必要，勿直接操作var *指针，除非你知道自己在做什么；游离在外的var *指针没有调用refer登记为引用的，相当于弱引用，垃圾回收时候会被删除；未分配的var *指针（指向NULL）在大部分函数里都会异常退出。
2. 将a变量赋值给b变量，实际传递的是地址；为了节省内存，null/true/false都是全局常量；所有赋number/string原始值的，都是创建新变量，而非修改原变量；array/object另有一套增删改的函数。
3. 对于异常处理，所有无法恢复的错误均执行exitif宏退出进程。
4. 我也忘了使用到C语言最新标准的什么特性，反正最新版本GCC编译通过就是了，包含MinGW的makefile，别的操作系统自行修改。
5. 这不是玩具，我的想法是用它替代C++/Rust之类走入歧途的复杂东西，反正我测试过程中还没有发现内存泄露等严重问题。
6. 函数命名规范：可选缩写+完整单词+可选后缀，缩写的含义如下，当前用到的后缀只有一个，_s表示字符串参数带长度参数的安全函数，C语言字符串字面量可以安全使用不带_s的函数，别的情况建议用带_s的。

|缩写|说明|
|-|-|
|v|动态类型的变量|
|r|引用|
|z|zero、zilch、zip，表示null，参见<https://www.englishtrackers.com/english-blog/zero-zilch-zip-nil-nought-nothing-whats-the-difference/>|
|b|boolean|
|n|number|
|s|string|
|a|array|
|o|object|
|w|world、whole，表示整体的、全局的|
|sb|stringbuffer辅助工具|

以下是函数/宏列表，以及对应的常见动态语言的语法（你一看就懂），和举例：

|函数/宏|动态语法|说明/例子|
|-|-|-|
|dump()||打印所有常量、变量和引用信息|
|void gc()||垃圾回收|
|vassign(a, b)|a = b||
|vdeclare(a, b)|var a = b||
|struct var *znew()||返回指向null常量的地址|
|zdeclare(a)|var a = null||
|struct var *bnew(bool b)||返回指向true/false常量的地址|
|bdeclare(a, b)|var a = true|bdeclare(a, true)|
|bool bvalue(struct var *pv)||返回boolean变量的原始值|
|struct var *nnew(double n)||创建number变量|
|ndeclare(a, b)|var a = 3.14|ndeclare(a, 3.14)|
|double nvalue(struct var *pv)||返回number变量的原始值|
|var *snew_s(const char *s, size_t slen)<br>var *snew(const char *sz)||创建string变量|
|sdeclare(a, b)|var a = "hello"|sdeclare(a, "hello")|
|char *svalue(struct var *pv)||返回string变量的原始值（指向\0结尾的字符数组）|
|size_t slength(struct var *pv)|a.length()|返回字符串长度|
|struct var *sconcat(size_t num, ...)|var a = "hi"<br>var b = "all"<br>var c = a + b|sdeclare(a, "hi")<br>sdeclare(b, "all")<br>vdeclare(c, sconcat(2, a, b))|
|var *anew(size_t num, ...)|var a = [null, true, 3.14, "hello"]|创建包含0个或多个成员的array变量<br>vdeclare(a, anew(4, znew(), bnew(true), nnew(3.14), snew("hello")))|
|adeclare(a)|var a = []||
|void aclear(struct var *pv)|v.clear()|清空数组|
|size_t alength(struct var *pv)|v.length()|返回数组长度|
|void apush(struct var *pv, struct var *pval)|v.push(val)|数组末端插入，长度+1|
|struct var *apop(struct var *pv)|v.pop()|数组末端弹出，长度-1|
|void aput(struct var *pv, size_t idx, struct var *pval)|v[idx] = val|数组指定下标写入替换原有值，下标越界则程序退出|
|struct var *aget(struct var *pv, size_t idx)|v[idx]|取数组指定下标的值，下标越界则程序退出|
|asort(....)||数组排序，未实现|
|struct var *onew()||创建空对象|
|odeclare(a)|var a = {}||
|void oclear(struct var *pv)|v.clear()|清空对象|
|size_t olength(struct var *pv)|v.length()|返回对象的元素数量|
|void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval)<br>void oput(struct var *pv, const char *key, struct var *pval)|v[key] = val|写入键值对|
|struct var *oget_s(struct var *pv, const char *key, size_t klen)<br>struct var *oget(struct var *pv, const char *key)|v[key]|读取键对应的值，注意可能返回NULL，程序务必要做判断|
|struct var *vtojson(struct var *pv)||json化任意变量，结果以字符串变量返回，注意：禁止套娃，循环嵌套的忽略|
|tojson(pv)|print([null, [3.140000, "hi"], false])|返回变量json化后的C字符数组，用于printf等函数<br>printf(tojson(anew(3, znew(), anew(2, nnew(3.14), snew("hi")), bnew(false))))|
|fromjson(...)||解析json字符串，构建变量，未实现。|

更多示例代码，参见test_var.c



