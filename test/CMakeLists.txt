CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(ModuleGateway)

SET(CMAKE_CXX_STANDARD 20)

ADD_SUBDIRECTORY("${CMAKE_CURRENT_LIST_DIR}/lib/example-module")

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-DDEBUG)
ENDIF()

IF(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(WARNING "The only tested working compiler is clang++-17. The current compiler is ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}. Consider change.")
ENDIF()

# GTest static librari is not compiled as pic
SET(CMAKE_POSITION_INDEPENDENT_CODE OFF)

FIND_PACKAGE(GTest REQUIRED)
FILE(GLOB_RECURSE source_test_files "${CMAKE_CURRENT_LIST_DIR}/source/*" "${CMAKE_CURRENT_LIST_DIR}/source/testing_utils/*")
ADD_EXECUTABLE(modulegateway_tests ${source_test_files} ${CMAKE_CURRENT_LIST_DIR}/mainTests.cpp)
TARGET_INCLUDE_DIRECTORIES(modulegateway_tests PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include/")
TARGET_LINK_LIBRARIES(modulegateway_tests PUBLIC ${GTEST_LIBRARIES} pthread module-gateway-lib)

TARGET_COMPILE_OPTIONS(modulegateway_tests PRIVATE -Wall -Wextra -Wpedantic -fsanitize=address)

INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(modulegateway_tests)
