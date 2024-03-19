CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)
PROJECT(ModuleGateway)

IF (TARGET memory_management)
    RETURN()
ENDIF ()

SET(CMAKE_CXX_STANDARD 20)


FILE(GLOB_RECURSE memory_management_cpp_files "./src/*")
ADD_LIBRARY(memory_management STATIC ${memory_management_cpp_files})
TARGET_LINK_LIBRARIES(memory_management PUBLIC fleet-protocol::common_headers)