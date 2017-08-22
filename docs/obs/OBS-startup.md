# OBS Studio启动关键流程分析

OBS Studio启动流程从obs-app.cpp的main函数开始：
```c
SetErrorMode(SEM_FAILCRITICALERRORS);
load_debug_privilege();
base_set_crash_handler(main_crash_handler, nullptr); 
base_get_log_handler(&def_log_handler, nullptr);
```
提升进程权限，设置进程崩溃处理模式和处理函数。没有什么好说的。
```c
for (int i = 1; i < argc; i++) {
    if (arg_is(argv[i], "--portable", "-p")) {
        portable_mode = true;

    } else if (arg_is(argv[i], "--multi", "-m")) {
        multi = true;

    } else if (arg_is(argv[i], "--verbose", nullptr)) {
        log_verbose = true;

    } else if (arg_is(argv[i], "--always-on-top", nullptr)) {
        opt_always_on_top = true;

    } else if (arg_is(argv[i], "--unfiltered_log", nullptr)) {
        unfiltered_log = true;

    } else if (arg_is(argv[i], "--startstreaming", nullptr)) {
        opt_start_streaming = true;

    } else if (arg_is(argv[i], "--startrecording", nullptr)) {
        opt_start_recording = true;

    } else if (arg_is(argv[i], "--startreplaybuffer", nullptr)) {
        opt_start_replaybuffer = true;

    } else if (arg_is(argv[i], "--collection", nullptr)) {
        if (++i < argc) opt_starting_collection = argv[i];

    } else if (arg_is(argv[i], "--profile", nullptr)) {
        if (++i < argc) opt_starting_profile = argv[i];

    } else if (arg_is(argv[i], "--scene", nullptr)) {
        if (++i < argc) opt_starting_scene = argv[i];

    } else if (arg_is(argv[i], "--minimize-to-tray", nullptr)) {
        opt_minimize_tray = true;

    } else if (arg_is(argv[i], "--studio-mode", nullptr)) {
        opt_studio_mode = true;

    } else if (arg_is(argv[i], "--allow-opengl", nullptr)) {
        opt_allow_opengl = true;

    } else if (arg_is(argv[i], "--help", "-h")) {
        std::cout <<
        "--help, -h: Get list of available commands.\n\n" << 
        "--startstreaming: Automatically start streaming.\n" <<
        "--startrecording: Automatically start recording.\n" <<
        "--startreplaybuffer: Start replay buffer.\n\n" <<
        "--collection <string>: Use specific scene collection."
            << "\n" <<
        "--profile <string>: Use specific profile.\n" <<
        "--scene <string>: Start with specific scene.\n\n" <<
        "--studio-mode: Enable studio mode.\n" <<
        "--minimize-to-tray: Minimize to system tray.\n" <<
        "--portable, -p: Use portable mode.\n" <<
        "--multi, -m: Don't warn when launching multiple instances.\n\n" <<
        "--verbose: Make log more verbose.\n" <<
        "--always-on-top: Start in 'always on top' mode.\n\n" <<
        "--unfiltered_log: Make log unfiltered.\n\n" <<
        "--allow-opengl: Allow OpenGL on Windows.\n\n" <<
        "--version, -V: Get current version.\n";

        exit(0);

    } else if (arg_is(argv[i], "--version", "-V")) {
        std::cout << "OBS Studio - " << 
            App()->GetVersionString() << "\n";
        exit(0);
    }
}
```
对命令行模式的参数进行解析，设置一些状态变量。如果不是从命令行启动的话，根本就不会执行这部分代码。
```c
#if !OBS_UNIX_STRUCTURE
	if (!portable_mode) {
		portable_mode =
			os_file_exists(BASE_PATH "/portable_mode") ||
			os_file_exists(BASE_PATH "/obs_portable_mode") ||
			os_file_exists(BASE_PATH "/portable_mode.txt") ||
			os_file_exists(BASE_PATH "/obs_portable_mode.txt");
	}
#endif
```
这部分代码主要是通过判断这四个文件是否存在`portable_mode`, `obs_portable_mode`, `portable_mode.txt`, `obs_portable_mode.txt`来确定，OBS当前是否是portable模式运行的。余下的代码就比较少了：
```c
upgrade_settings();

fstream logFile;

curl_global_init(CURL_GLOBAL_ALL);
int ret = run_program(logFile, argc, argv);

blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
base_set_log_handler(nullptr, nullptr);
```
从函数名称来看，upgrade_settings()主要是用来更新配置文件的，`curl_global_init`初始化libcurl库，而`blog`用于输出内存泄漏情况。怎么统计的呢？这里初步推测是用了引用计数方法。每分配一块内存就加1，每释放一块内存就减去1，最后如果引用数大于1，那么就是没有释放的内存块数了，也就是内存泄露的大小了。最后`base_set_log_handler`方法释放日志处理函数相关资源。
那么，这里就着重看两个函数了：`upgrade_settings`和`run_program`函数。先来看看配置文件更新操作是怎么一回事吧：
```c
char path[512];
int pathlen = GetConfigPath(path, 512, "obs-studio/basic/profiles");
```
可以看到函数一开始就调用`GetConfigPath`获取配置文件`obs-studio/basic/profiles`路径。`GetConfigPath`的实现如下：
```c
int GetConfigPath(char *path, size_t size, const char *name)
{
	if (!OBS_UNIX_STRUCTURE && portable_mode) {
		if (name && *name) {
			return snprintf(path, size, CONFIG_PATH "/%s", name);
		} else {
			return snprintf(path, size, CONFIG_PATH);
		}
	} else {
		return os_get_config_path(path, size, name);
	}
}
```
我们只看非portable模式运行的情况，直接调用`obs_get_config_path`方法：
```c
int os_get_config_path(char *dst, size_t size, const char *name)
{
	return os_get_path_internal(dst, size, name, CSIDL_APPDATA);
}
```
内部直接调用`obs_get_path_internal`方法：
```c
/* returns [folder]\[name] on windows */
static int os_get_path_internal(char *dst, size_t size, const char *name, int folder)
{
	wchar_t path_utf16[MAX_PATH];

	SHGetFolderPathW(NULL, folder, NULL, SHGFP_TYPE_CURRENT, path_utf16);

	if (os_wcs_to_utf8(path_utf16, 0, dst, size) != 0) {
		if (!name || !*name) {
			return (int)strlen(dst);
		}

		if (strcat_s(dst, size, "\\") == 0) {
			if (strcat_s(dst, size, name) == 0) {
				return (int)strlen(dst);
			}
		}
	}

	return -1;
}

```
这个方法首先通过`SHGetFolderPathW`获取windows上的用户数据文件夹，在windows上的路径为：`C:\Users\[user name]\AppData\Roaming`。这个路径是通过`CSIDL_APPDATA`指定的。不过值得注意的是，`SHGetFolderPathW`已经被弃用，已经被更新的`SHGetKnownFolderPath`所代替。但是`SHGetKnownFolderPath`只支持Vista之后的系统，要支持XP的话还得继续使用`SHGetFolderPathW`函数。
函数剩下的工作就是调用`strcat_s`方法拼接字符串路径了，最终得到了完整的配置文件路径：`C:\Users\[user name]\AppData\Roaming\obs-studio\basic\profiles`，并返回路径长度。

得到配置文件路径之后，流程继续回到`upgrade_settings`函数，只剩下关键部分while循环代码。大致流程是一个一个枚举上面获取的文件夹下面的文件夹项，拼接路径然后解析配置文件。首先排除当前目录`.`和上级目录`..`,
```c
if (ent->directory && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
```
然后拼接字符串，得到基本配置文件basic.ini的路径。在我的电脑上为：
```c
C:\Users\zhang\AppData\Roaming\obs-studio\basic\profiles\未命名\basic.ini
```
得到配置文件后即可打开进行解析：
```c
ConfigFile config;
int ret;

ret = config.Open(path, CONFIG_OPEN_EXISTING);
if (ret == CONFIG_SUCCESS) {
    if (update_ffmpeg_output(config) ||
        update_reconnect(config)) {
        config_save_safe(config, "tmp", nullptr);
    }
}

```
这里主要工作分成三部分：（1）ffmpeg视频流编解码相关配置。（2）断线重连配置。（3）保存配置。
（1）ffmpeg视频流编码配置，主要是解析`AdvOut`段的配置：
```c
static bool update_ffmpeg_output(ConfigFile &config)
{
	if (config_has_user_value(config, "AdvOut", "FFOutputToFile"))
		return false;

	const char *url = config_get_string(config, "AdvOut", "FFURL");
	if (!url)
		return false;

	bool isActualURL = strstr(url, "://") != nullptr;
	if (isActualURL)
		return false;

	string urlStr = url;
	string extension;

	for (size_t i = urlStr.length(); i > 0; i--) {
		size_t idx = i - 1;

		if (urlStr[idx] == '.') {
			extension = &urlStr[i];
		}

		if (urlStr[idx] == '\\' || urlStr[idx] == '/') {
			urlStr[idx] = 0;
			break;
		}
	}

	if (urlStr.empty() || extension.empty())
		return false;

	config_remove_value(config, "AdvOut", "FFURL");
	config_set_string(config, "AdvOut", "FFFilePath", urlStr.c_str());
	config_set_string(config, "AdvOut", "FFExtension", extension.c_str());
	config_set_bool(config, "AdvOut", "FFOutputToFile", true);
	return true;
}
```
(2)短线重连配置
```c
static bool update_reconnect(ConfigFile &config)
{
	if (!config_has_user_value(config, "Output", "Mode"))
		return false;

	const char *mode = config_get_string(config, "Output", "Mode");
	if (!mode)
		return false;

	const char *section = (strcmp(mode, "Advanced") == 0) ?
		"AdvOut" : "SimpleOutput";

	if (move_reconnect_settings(config, section)) {
		config_remove_value(config, "SimpleOutput", "Reconnect");
		config_remove_value(config, "SimpleOutput", "RetryDelay");
		config_remove_value(config, "SimpleOutput", "MaxRetries");
		config_remove_value(config, "AdvOut", "Reconnect");
		config_remove_value(config, "AdvOut", "RetryDelay");
		config_remove_value(config, "AdvOut", "MaxRetries");
		return true;
	}

	return false;
}
```
代码上初步看是从`SimpleOutput`和`AdvOut`段移除关于重连的配置，至于为什么要这么做现在还不清楚。
剩下的代码是生成两个配置文件`recordEncoder.json`和`streamEncoder.json`。按照字面意思理解，分别用于配置录制视频和推流时的编码器。
```c
if (config) {
    const char *sEnc = config_get_string(config,
            "AdvOut", "Encoder");
    const char *rEnc = config_get_string(config,
            "AdvOut", "RecEncoder");

    /* replace "cbr" option with "rate_control" for
        * each profile's encoder data */
    path[pathlen] = 0;
    strcat(path, "/");
    strcat(path, ent->d_name);
    strcat(path, "/recordEncoder.json");
    convert_14_2_encoder_setting(rEnc, path);

    path[pathlen] = 0;
    strcat(path, "/");
    strcat(path, ent->d_name);
    strcat(path, "/streamEncoder.json");
    convert_14_2_encoder_setting(sEnc, path);
}

```
配置文件的更新就到这里结束了。具体的配置含义和为什么要这么做，还需要配合用户界面的响应代码来理解。先往后面走着，来看看`run_program`这个函数，最终完成了哪些初始化工作吧。`run_prograsm`函数完成了程序对象的初始化，多实例的检测，国际化文本处理器QTranslator植入等工作。这里主要来分析下程序对象的初始化和多实例运行检测。
程序对象`OBSApp`的初始化方法`AppInit`主要代码如下：（排除干扰代码）
```c
if (!InitApplicationBundle())
    throw "Failed to initialize application bundle";
if (!MakeUserDirs())
    throw "Failed to create required user directories";
if (!InitGlobalConfig())
    throw "Failed to initialize global config";
if (!InitLocale())
    throw "Failed to load locale";
if (!InitTheme())
    throw "Failed to load theme";

move_basic_to_profiles();
move_basic_to_scene_collections();

if (!MakeUserProfileDirs())
    throw "Failed to create profile directories";
```
其中，`InitApplicationBundle`暂时没有什么功能，直接返回了true.`MakeUserDirs`用于创建用户数据目录：
```c
static bool MakeUserDirs()
{
	char path[512];

	if (GetConfigPath(path, sizeof(path), "obs-studio/basic") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetConfigPath(path, sizeof(path), "obs-studio/logs") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetConfigPath(path, sizeof(path), "obs-studio/profiler_data") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

#ifdef _WIN32
	if (GetConfigPath(path, sizeof(path), "obs-studio/crashes") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetConfigPath(path, sizeof(path), "obs-studio/updates") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;
#endif

	if (GetConfigPath(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	return true;
}
```
主要包括`obs-studio/basic`,`obs-studio/logs`,`obs-studio/profiler_data`,`obs-studio/plugin_config`,`obs-studio/crashes`,`obs-studio/updates`这几个文件夹。其中，最后两个文件夹只针对windows创建，因为和崩溃日志、程序更新相关，在mac上没有相关功能，因此使用了条件编译。

`InitGlobalConfig`函数主要是创建`global.ini`文件。这个是全局配置文件，具体的代码就不细看了，反正就是设置各种默认值的。
`InitLocale`和国际化语言相关。
`InitTheme`设置主题相关，其实是加载qss文件实现界面的自定义

`MakeUserProfileDirs`主要用于创建两个文件夹(`obs-studio/basic/profiles`和`obs-studio/basic/scenes`)：
```c
static bool MakeUserProfileDirs()
{
	char path[512];

	if (GetConfigPath(path, sizeof(path), "obs-studio/basic/profiles") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetConfigPath(path, sizeof(path), "obs-studio/basic/scenes") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	return true;
}
```

`AppInit`方法看完了，接下来就看看最重要的另外一个方法`OBSInit`方法：
```c
bool OBSApp::OBSInit()
{
	ProfileScope("OBSApp::OBSInit");

	bool licenseAccepted = config_get_bool(globalConfig, "General", "LicenseAccepted");
	OBSLicenseAgreement agreement(nullptr);

	if (licenseAccepted || agreement.exec() == QDialog::Accepted) {
		if (!licenseAccepted) {
			config_set_bool(globalConfig, "General", "LicenseAccepted", true);
			config_save(globalConfig);
		}

		if (!StartupOBS(locale.c_str(), GetProfilerNameStore()))
			return false;

		blog(LOG_INFO, "Portable mode: %s", portable_mode ? "true" : "false");

		setQuitOnLastWindowClosed(false);

		mainWindow = new OBSBasic();

		mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(mainWindow, SIGNAL(destroyed()), this, SLOT(quit()));

		mainWindow->OBSInit();

		connect(this, &QGuiApplication::applicationStateChanged,
				[](Qt::ApplicationState state)
				{
					obs_hotkey_enable_background_press(
						state != Qt::ApplicationActive);
				});
		obs_hotkey_enable_background_press(
				applicationState() != Qt::ApplicationActive);
		return true;
	} else {
		return false;
	}
}
```
代码逻辑非常简单了，首先显示一个许可证对话框。当用户同意许可证时再继续往下初始化，否则程序就直接结束了。 如果用户同意了许可证，那么逻辑也很简单，首先调用OBSStartup()方法，然后初始化主窗口类OBSBasic。 OBSStarup()方法又包含一个非常庞大的执行逻辑。现在就慢慢来分析下：







