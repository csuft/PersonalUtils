#include "profile.h"

#if ENABLE_API_PROFILER

static const float APIProfiler_ReportIntervalSecs = 1.0f;

float APIProfiler::s_ooFrequency = 0;
INT64 APIProfiler::s_reportInterval = 0;

//------------------------------------------------------------------
// Flush is called at the rate determined by APIProfiler_ReportIntervalSecs
//------------------------------------------------------------------
void APIProfiler::Flush(INT64 end)
{
    // Auto-initialize globals based on timer frequency:
    if (s_reportInterval == 0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        s_ooFrequency = 1.0f / freq.QuadPart;
        MemoryBarrier();
        s_reportInterval = (INT64) (freq.QuadPart * APIProfiler_ReportIntervalSecs);
    }

    // Avoid garbage timing on first call by initializing a new interval:
    if (m_threadInfo->lastReportTime == 0)
    {
        m_threadInfo->lastReportTime = m_start;
        return;
    }

    // Enough time has elapsed. Print statistics to console:
    float interval = (end - m_threadInfo->lastReportTime) * s_ooFrequency;
    float measured = m_threadInfo->accumulator * s_ooFrequency;
    printf("TID 0x%x time spent in \"%s\": %.0f/%.0f ms %.1f%% %dx\n",
        GetCurrentThreadId(),
        m_threadInfo->name,
        measured * 1000,
        interval * 1000,
        100.f * measured / interval,
        m_threadInfo->hitCount);

    // Reset statistics and begin next timing interval:
    m_threadInfo->lastReportTime = end;
    m_threadInfo->accumulator = 0;
    m_threadInfo->hitCount = 0;
}

#endif