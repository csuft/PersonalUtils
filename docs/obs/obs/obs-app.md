# OBS Studio源码解读之main函数启动过程

OBS启动函数main()里面做了不少的事情，包括程序崩溃处理机制、日志处理、命令行参数解析、配置文件初始化等。下面就来看看其中的流程：
```c
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS);
    load_debug_privilege();
    base_set_crash_handler(main_crash_handler, nullptr);
#endif
```
这一段代码只在Windows平台运行。`SetErrorMode`方法设置程序出错时是否弹出对话框。在这里设置为了`SEM_FAILCRITICALERRORS`，程序出错时将不会弹出错误提示框，系统将把错误信息发送给调用进程。随后`load_debug_privilege`方法具体内容如下：
```c
static void load_debug_privilege(void)
{
    const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    TOKEN_PRIVILEGES tp;
    HANDLE token;
    LUID val;

    if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
        return;
    }

    if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = val;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL, NULL);
    }

    CloseHandle(token);
}
```
这段代码中出现了3个Windows API调用：`OpenProcessToken`,`LookupPrivilegeValue`，`AdjustTokenPrivileges`。这三个API通常配合在一起用来提升进程权限。至于其作用现在还不明确，等后面的解析吧。
`base_set_crash_handler`方法指定了程序的崩溃处理函数`main_crash_handler`。这是一个回调函数，具体代码如下：
```c
static void main_crash_handler(const char *format, va_list args, void *param)
{
    char *text = new char[MAX_CRASH_REPORT_SIZE];

    vsnprintf(text, MAX_CRASH_REPORT_SIZE, format, args);
    text[MAX_CRASH_REPORT_SIZE - 1] = 0;

    delete_oldest_file("obs-studio/crashes");

    string name = "obs-studio/crashes/Crash ";
    name += GenerateTimeDateFilename("txt");

    BPtr<char> path(GetConfigPathPtr(name.c_str()));

    fstream file;

#ifdef _WIN32
    BPtr<wchar_t> wpath;
    os_utf8_to_wcs_ptr(path, 0, &wpath);
    file.open(wpath, ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#else
    file.open(path,	ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#endif
    file << text;
    file.close();

    int ret = MessageBoxA(NULL, CRASH_MESSAGE, "OBS has crashed!", MB_YESNO | MB_ICONERROR | MB_TASKMODAL);

    if (ret == IDYES) {
        size_t len = strlen(text);

        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, len);
        memcpy(GlobalLock(mem), text, len);
        GlobalUnlock(mem);

        OpenClipboard(0);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, mem);
        CloseClipboard();
    }

    exit(-1);

    UNUSED_PARAMETER(param);
}
```
这段代码基本上干了两件事。第一件事就是创建崩溃日志文件，然后将崩溃信息格式化保存到日志文件中去。在这个过程中，程序对windows和mac平台做了区分，windows上将文件路径转换成了宽字符路径使用:
```c
#ifdef _WIN32
    BPtr<wchar_t> wpath;
    os_utf8_to_wcs_ptr(path, 0, &wpath);
    file.open(wpath, ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#else
	file.open(path,	ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#endif
```
剩下的第二件事就是把崩溃信息打入系统剪贴板啦：
```c
int ret = MessageBoxA(NULL, CRASH_MESSAGE, "OBS has crashed!", MB_YESNO | MB_ICONERROR | MB_TASKMODAL);

if (ret == IDYES) {
    size_t len = strlen(text);

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(mem), text, len);
    GlobalUnlock(mem);

    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, mem);
    CloseClipboard();
}
```
接下来的工作就是这是日志处理函数了：
```c
base_get_log_handler(&def_log_handler, nullptr);
```
这里设置的默认处理函数，其实现如下：

```c
static void def_log_handler(int log_level, const char *format, va_list args, void *param)
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
```
然后，代码流程就进入OBSApp模块中去了。OBSApp继承自Qt中的QApplication类，是应用程序的抽象，主要是用户界面相关的初始化了。





