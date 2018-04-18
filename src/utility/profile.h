#define ENABLE_API_PROFILER 1     // Comment this line to disable the profiler

#if ENABLE_API_PROFILER

//------------------------------------------------------------------
// A class for local variables created on the stack by the API_PROFILER macro:
//------------------------------------------------------------------
class APIProfiler
{
public:
    //------------------------------------------------------------------
    // A structure for each thread to store information about an API:
    //------------------------------------------------------------------
    struct ThreadInfo
    {
        INT64 lastReportTime;
        INT64 accumulator;   // total time spent in target module since the last report
        INT64 hitCount;      // number of times the target module was called since last report
        const char *name;    // the name of the target module
    };

private:
    INT64 m_start;
    ThreadInfo *m_threadInfo;

    static float s_ooFrequency;      // 1.0 divided by QueryPerformanceFrequency()
    static INT64 s_reportInterval;   // length of time between reports
    void Flush(INT64 end);
    
public:
    __forceinline APIProfiler(ThreadInfo *threadInfo)
    {
        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
        m_start = start.QuadPart;
        m_threadInfo = threadInfo;
    }

    __forceinline ~APIProfiler()
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        m_threadInfo->accumulator += (end.QuadPart - m_start);
        m_threadInfo->hitCount++;
        if (end.QuadPart - m_threadInfo->lastReportTime > s_reportInterval)
            Flush(end.QuadPart);
    }
};

//----------------------
// Profiler is enabled
//----------------------
#define DECLARE_API_PROFILER(name) \
    extern __declspec(thread) APIProfiler::ThreadInfo __APIProfiler_##name;

#define DEFINE_API_PROFILER(name) \
    __declspec(thread) APIProfiler::ThreadInfo __APIProfiler_##name = { 0, 0, 0, #name };

#define TOKENPASTE2(x, y) x ## y
#define TOKENPASTE(x, y) TOKENPASTE2(x, y)
#define API_PROFILER(name) \
    APIProfiler TOKENPASTE(__APIProfiler_##name, __LINE__)(&__APIProfiler_##name)

#else

//----------------------
// Profiler is disabled
//----------------------
#define DECLARE_API_PROFILER(name)
#define DEFINE_API_PROFILER(name)
#define API_PROFILER(name)

#endif