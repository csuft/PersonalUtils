# OBS Studio源码走读之日志与崩溃处理函数

本模块对程序的日志处理函数和崩溃处理函数进行设置，直接看代码：

```c
// 宏定义简化代码
typedef void (*log_handler_t)(int lvl, const char *msg, va_list args, void *p);

EXPORT void base_get_log_handler(log_handler_t *handler, void **param);
EXPORT void base_set_log_handler(log_handler_t handler, void *param);

EXPORT void base_set_crash_handler(
		void (*handler)(const char *, va_list, void *),
		void *param);

```
头文件声明了几个设置处理器函数的接口，接下来定义了默认的日志和崩溃处理函数：

```c
static void def_log_handler(int log_level, const char *format,
		va_list args, void *param)
{
	char out[4096];
	vsnprintf(out, sizeof(out), format, args);

	if (log_level <= log_output_level) {
		switch (log_level) {
		case LOG_DEBUG:
			fprintf(stdout, "debug: %s\n", out);
			fflush(stdout);
			break;

		case LOG_INFO:
			fprintf(stdout, "info: %s\n", out);
			fflush(stdout);
			break;

		case LOG_WARNING:
			fprintf(stdout, "warning: %s\n", out);
			fflush(stdout);
			break;

		case LOG_ERROR:
			fprintf(stderr, "error: %s\n", out);
			fflush(stderr);
		}
	}

	UNUSED_PARAMETER(param);
}

NORETURN static void def_crash_handler(const char *format, va_list args,
		void *param)
{
	vfprintf(stderr, format, args);
	exit(0);

	UNUSED_PARAMETER(param);
}
```
给程序设置默认的处理器：

```c
static log_handler_t log_handler = def_log_handler;
static void (*crash_handler)(const char *, va_list, void *) = def_crash_handler;
```  

`log_handler`和`crash_handler`都被`static`修饰，因此只在本源文件访问到，其他源文件无法访问到，相当于类的私有成员。在C语言中，static修饰的函数也具备类似的属性，即只能在本源文件中调用，在其他的文件中是访问不到的。  
如果默认的处理函数无法满足要求，可以自行设置处理函数：
```c
static void *log_param   = NULL;
static void *crash_param = NULL;
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
```
最后封装了变参数接口来实现日志功能：
```c
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
外部直接调用`blog()`函数，`blog()`函数内部调用`blogva()`，`blogva()`内部又调用`log_handler()`实现日志处理。





