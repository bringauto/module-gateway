CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)
PROJECT(ModuleGateway)

IF (TARGET memory-management)
    RETURN()
ENDIF ()

SET(CMAKE_CXX_STANDARD 20)


FILE(GLOB_RECURSE memory_management_cpp_files "./src/*")
ADD_LIBRARY(memory-management STATIC ${memory_management_cpp_files})
TARGET_LINK_LIBRARIES(memory-management PUBLIC fleet-protocol-interface::common-headers-interface)