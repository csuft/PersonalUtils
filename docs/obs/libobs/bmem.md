# OBS Studio源码走读之内存管理函数(bmem)
这个模块主要是对内存操作函数进行了一定的封装。代码中首先定义了内存分配器结构体：
```c
struct base_allocator {
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
};
```
该结构体包含三个函数指针：malloc, realloc, free，分别对应C语言中的三个内存管理函数。在实现文件中，首先定义了一个全局静态变量，其中三个成员变量分别赋值成a_malloc, a_realloc, a_free：
```c
static struct base_allocator alloc = {a_malloc, a_realloc, a_free};
```
其中，a_malloc, a_realloc, a_free分别定义如下：
```c
static void *a_malloc(size_t size)
{
#ifdef ALIGNED_MALLOC
    return _aligned_malloc(size, ALIGNMENT);
#elif ALIGNMENT_HACK
    void *ptr = NULL;
    long diff;

    ptr  = malloc(size + ALIGNMENT);
    if (ptr) {
        diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
        ptr  = (char *)ptr + diff;
        ((char *)ptr)[-1] = (char)diff;
    }

    return ptr;
#else
    return malloc(size);
#endif
}

static void *a_realloc(void *ptr, size_t size)
{
#ifdef ALIGNED_MALLOC
    return _aligned_realloc(ptr, size, ALIGNMENT);
#elif ALIGNMENT_HACK
    long diff;

    if (!ptr)
        return a_malloc(size);
    diff = ((char *)ptr)[-1];
    ptr = realloc((char*)ptr - diff, size + diff);
    if (ptr)
        ptr = (char *)ptr + diff;
    return ptr;
#else
    return realloc(ptr, size);
#endif
}

static void a_free(void *ptr)
{
#ifdef ALIGNED_MALLOC
    _aligned_free(ptr);
#elif ALIGNMENT_HACK
    if (ptr)
        free((char *)ptr - ((char*)ptr)[-1]);
#else
    free(ptr);
#endif
}
```  
可以看到，这三个函数内部在进行内存管理时都考虑到了内存对齐的问题。在Windows平台上分别调用 **\_aligned_malloc()** , **\_aligned_realloc()**, **\_aligned_free()** 进行对齐处理(32位对齐)，无需更多的操作。在其他的平台则只能通过hack技巧来实现了。如果对内存对齐没有要求，那么直接就调用 **malloc()** , **realloc()** , **free()** 完成了事。  
## 内存对齐
那么，上面的内存对齐hack如怎么进行的呢？先来看看**a_malloc()**的实现：
```c
void *ptr = NULL;
long diff;

ptr  = malloc(size + ALIGNMENT);
if (ptr) {
    diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
    ptr  = (char *)ptr + diff;
    ((char *)ptr)[-1] = (char)diff;
}

return ptr;
```
首先还是调用**malloc()** 函数分配一块 **size+ALIGNMENT**大小的内存。之所以分配的内存要比请求的大小大**ALIGNMENT**，是出于对内存对齐的考虑。因为**malloc**返回的内存块起始地址不一定是ALIGNMENT的整数倍，需要进行对其操作，这里就是为了预留空间。  
然后，计算对齐操作需要的偏移：
```c
diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
ptr  = (char *)ptr + diff;
```
这里先将原内存地址值进行按位取反操作，然后与(ALIGNMENT-1)按位与操作，最后加1得出diff偏移值。具体计算以下为例：
```bash
# original memory address
00000000 00000000 01000100 00100110
# bitwise NOT
11111111 11111111 10111011 11011001
# bitwise AND with (ALIGNMENT - 1)
11111111 11111111 10111011 11011001 & 00000000 00000000 00000000 00011111
# temperary value
00000000 00000000 00000000 00011001
# temperary value plus 1 is the 26
00000000 00000000 00000000 00011010
```
最终diff的值为26，也就是说原内存地址需要偏移26字节，才能实现32字节对齐。值得注意的是下一句代码：
```c
((char *)ptr)[-1] = (char)diff;
```
这一句代码将偏移值存储到了已经进行内存对齐的地址的前一个单元。这里是方便后面realloc, free的实现。
> 因为C语言指针可以通过加减整型数前后移动，同时指针也可以通过数组下标的方式来引用元素，因此这里的负数下标是合法的。

再看看**a_realloc**函数的实现吧：
```c
long diff;

if (!ptr)
    return a_malloc(size);
diff = ((char *)ptr)[-1];
ptr = realloc((char*)ptr - diff, size + diff);
if (ptr)
    ptr = (char *)ptr + diff;
return ptr;
```  
如果调用a_realloc()时指针还是空的，直接调用a_malloc()函数分配内存。否则先把diff值取出来，释放掉内存再重新分配：
```c
ptr = realloc((char*)ptr - diff, size + diff);
```
注意：这里**ptr-diff**得到是原始内存地址，而不是经过内存对齐操作的地址，这样才不会出现内存泄漏。size+diff是请求重新分配的内存大小。这里直接加上diff了，而不是ALIGNMENT值。这样不会有多余的内存浪费。

**a_free**函数的实现则非常简单：
```c
free((char *)ptr - ((char*)ptr)[-1]);
```
先取出diff值，然后即可计算出原始的内存地址，调用free即可释放。


## 其他函数
**base_set_allocator**函数： 
```c
void base_set_allocator(struct base_allocator *defs)
{
    memcpy(&alloc, defs, sizeof(struct base_allocator));
}
```
设置全局静态变量**base_allocator alloc**的值。    

**bmalloc** 函数：  
```c
void *bmalloc(size_t size)
{
    void *ptr = alloc.malloc(size);
    if (!ptr && !size)
        ptr = alloc.malloc(1);
    if (!ptr) {
        os_breakpoint();
        bcrash("Out of memory while trying to allocate %lu bytes", (unsigned long)size);
    }

    os_atomic_inc_long(&num_allocs);
    return ptr;
}
```
该函数首先尝试分配指定大小的内存。如果指定的内存大小为0且分配地址为0,则只分配一个字节的内存；如果返回的内存地址为空，则直接输出异常信息，终止程序。其中，**os_breakpoint**实现如下：
```c
#include <intrin.h>
void os_breakpoint(void)
{
    __debugbreak();
}
```
该函数将中断程序执行并弹出对话框，给用户选择是否执行调试器进行调试。

**brealloc()** 函数：
```c
void *brealloc(void *ptr, size_t size)
{
    if (!ptr)
        os_atomic_inc_long(&num_allocs);

    ptr = alloc.realloc(ptr, size);
    if (!ptr && !size)
        ptr = alloc.realloc(ptr, 1);
    if (!ptr) {
        os_breakpoint();
        bcrash("Out of memory while trying to allocate %lu bytes", (unsigned long)size);
    }

    return ptr;
}
```
此函数逻辑跟**bmalloc**函数相差不大，不细说。

**bfree**函数：
```c
void bfree(void *ptr)
{
    if (ptr)
        os_atomic_dec_long(&num_allocs);
    alloc.free(ptr);
}
```
释放内存，同时也减去内存分配计数。**num_allocs**变量用于统计已内存分配数。

**bmemdup**函数：内存复制
```c
void *bmemdup(const void *ptr, size_t size)
{
    void *out = bmalloc(size);
    if (size)
        memcpy(out, ptr, size);

    return out;
}
```
余下的是一些字符串复制函数：
```c
static inline char *bstrdup_n(const char *str, size_t n)
{
    char *dup;
    if (!str)
         return NULL;

    dup = (char*)bmemdup(str, n+1);
    dup[n] = 0;

    return dup;
}

static inline wchar_t *bwstrdup_n(const wchar_t *str, size_t n)
{
    wchar_t *dup;
    if (!str)
        return NULL;

    dup = (wchar_t*)bmemdup(str, (n+1) * sizeof(wchar_t));
    dup[n] = 0;

    return dup;
}

static inline char *bstrdup(const char *str)
{
    if (!str)
        return NULL;

    return bstrdup_n(str, strlen(str));
}

static inline wchar_t *bwstrdup(const wchar_t *str)
{
    if (!str)
        return NULL;

    return bwstrdup_n(str, wcslen(str));
}

```







