# OBS Studio源码解读之base.c模块

`base.c`文件的主要内容是日志处理和崩溃处理，也即设置好默认的日志记录函数和崩溃处理函数。具体如何来看代码吧。文件结构非常简单，首先定义了几个`static`成员:
```c
static int log_output_level = LOG_DEBUG; 
static int  crashing     = 0;
static void *log_param   = NULL;
static void *crash_param = NULL;

static void def_log_handler(int log_level, const char *format, va_list args, void *param);
static void def_crash_handler(const char *format, va_list args, void *param);
static log_handler_t log_handler = def_log_handler;
static void (*crash_handler)(const char *, va_list, void *) = def_crash_handler;

```
几个主要的成员包括：`log_param`, `log_handler`, `crash_param`, `crash_handler`, `def_log_handler`, `def_crash_handler`。其中，`def_log_handler`和`def_crash_handler`是默认处理函数，将函数指针分别赋给了`log_handler`和`crash_handler`指针变量。也就说，如果外部没有设置处理函数，程序将调用默认的处理。在C语言中，`static`声明的全局变量和函数具备文件作用域。也就是说，它们只能在其声明的文件中被访问。在其他的文件中是访问不到的。从这种意义上来说，非常类似于C++中的private作用域。
正因为如此，在这个模块中还定义了一些操作这些变量的函数：
```c
void base_get_log_handler(log_handler_t *handler, void **param)
{
	if (handler)
		*handler = log_handler;
	if (param)
		*param = log_param;
}

void base_set_log_handler(log_handler_t handler, void *param)
{
	if (!handler)
		handler = def_log_handler;

	log_param   = param;
	log_handler = handler;
}

void base_set_crash_handler(
		void (*handler)(const char *, va_list, void *),
		void *param)
{
	crash_param   = param;
	crash_handler = handler;
}

void bcrash(const char *format, ...)
{
	va_list args;

	if (crashing) {
		fputs("Crashed in the crash handler", stderr);
		exit(2);
	}

	crashing = 1;
	va_start(args, format);
	crash_handler(format, args, crash_param);
	va_end(args);
}

void blogva(int log_level, const char *format, va_list args)
{
	log_handler(log_level, format, args, log_param);
}

void blog(int log_level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	blogva(log_level, format, args);
	va_end(args);
}

```
`base_get_log_handler`和`base_get_log_handler`分别用于获取和设置当前的日志处理函数，操作的数据对象就是前面定义的static变量`log_handler`和`log_param`。而`base_set_crash_handler`则用于设置崩溃处理函数，操作的数据对象为`crash_handler`和`crash_param`。其他模块在使用的时候，只需要调用`bcrash`和`blog`这两个接口就可以了。
`bcrash`函数底层实际上就是对`crash_handler`的调用，这个static变量在前面已经通过`base_set_crash_handler`设置好了。因此，这里只是封装了变参数，实现多参数的传递调用。`blog`底层调用了`blogva`，也封装了变参数，然后调用`log_handler`输出日志。
























