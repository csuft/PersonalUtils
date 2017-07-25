#include "platform.h"

#include <iostream>
#include <chrono>
#include <string>
#include <time.h>

std::string getCurrentTimeString()
{
	using namespace std::chrono;

	struct tm tstruct;
	char buffer[80];

	auto tp = system_clock::now();
	auto now = system_clock::to_time_t(tp);
	tstruct = *localtime(&now);

	size_t written = strftime(buffer, sizeof(buffer), "%X", &tstruct);
	if (std::ratio_less<system_clock::period, seconds::period>::value && written && (sizeof(buffer) - written) > 5)
	{
		auto tp_secs = time_point_cast<seconds>(tp);
		auto millis = duration_cast<milliseconds>(tp - tp_secs).count();
		snprintf(buffer + written, sizeof(buffer) - written, ".%03u", static_cast<unsigned>(millis));
	}

	return buffer;
}