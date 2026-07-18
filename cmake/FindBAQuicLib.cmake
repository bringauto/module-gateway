include_guard(GLOBAL)

# Provides: ba-quic-lib::ba-quic-lib
# Resolution order: in-scope target -> system config package -> FetchContent source build.
#
# Shared, transport-only MsQuic client wrapper that replaces this repo's hand-rolled
# QuicCommunication internals.

if(TARGET ba-quic-lib::ba-quic-lib)
    set(BAQuicLib_FOUND TRUE)
    return()
endif()

if(ba-quic-lib_DIR MATCHES "${CMAKE_BINARY_DIR}")
    unset(ba-quic-lib_DIR CACHE)
endif()
find_package(ba-quic-lib QUIET CONFIG)
if(ba-quic-lib_FOUND)
    message(STATUS "[BA] ba-quic-lib: found via system package")
    set(BAQuicLib_FOUND TRUE)
    return()
endif()

message(STATUS "[BA] ba-quic-lib: system package not found, fetching via FetchContent")
include(FetchContent)
# TODO: ba-quic-lib has no version tag yet -- swap GIT_TAG to a real vX.Y.Z once one exists,
# instead of tracking the master branch head.
FetchContent_Declare(ba-quic-lib
    GIT_REPOSITORY https://gitlab.bringauto.com/bring-auto/libraries/quic-lib.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    OVERRIDE_FIND_PACKAGE)
# ba-quic-lib's own CMakeLists.txt declares an `option(BRINGAUTO_TESTS ...)` with the same name as
# this repo's. Since FetchContent pulls it in via add_subdirectory, an already-cached
# BRINGAUTO_TESTS=ON from *this* repo would otherwise also build ba-quic-lib's own test suite
# (pulling in gtest, generating its test certs, etc.) as an unwanted side effect. Shadow it with a
# plain (non-cache) variable for the duration of this add_subdirectory only -- CMake resolves the
# nearest-scope normal variable before falling back to the cache entry, and a child directory
# inherits the parent's normal-variable values at the point it's added.
set(_ba_quic_lib_saved_tests_flag "${BRINGAUTO_TESTS}")
set(BRINGAUTO_TESTS OFF)
FetchContent_MakeAvailable(ba-quic-lib)
set(BRINGAUTO_TESTS "${_ba_quic_lib_saved_tests_flag}")
unset(_ba_quic_lib_saved_tests_flag)
set(BAQuicLib_FOUND TRUE)
