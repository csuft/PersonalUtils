#include <iostream>
#include <string>
#include "../src/utility/platform.h"

#include <gtest/gtest.h>

TEST(Utility, CurrentTimeString)
{
	std::string timeStr = getCurrentTimeString();
	std::cout << timeStr << std::endl;
}

TEST(Utility, CurrentDateTime)
{
	std::string dateTimeStr = getCurrentDateTime();
	std::cout << dateTimeStr << std::endl;
}