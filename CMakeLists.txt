CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

PROJECT(OpenBase)

#################################################################
# CMAKE search path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib/gtest/${CMAKE_CFG_INTDIR}/)

#################################################################
# projects
SET(UTILITY_TESTS
	${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/test/utility.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/utility/platform.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utility/platform.cpp)
	
SET(UTILITY_LIBS
	gtest.lib)

#################################################################
# Compile targets	
ADD_EXECUTABLE(MainTest ${UTILITY_TESTS})
TARGET_LINK_LIBRARIES(MainTest ${UTILITY_LIBS})
	
	