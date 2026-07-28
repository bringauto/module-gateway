# Provides: ba-quic-lib::ba-quic-lib
# Resolution order: in-scope target -> system config package -> FetchContent source build.
#
# Shared, transport-only MsQuic client wrapper that replaces this repo's hand-rolled
# QuicCommunication internals.

if(TARGET ba-quic-lib::ba-quic-lib)
    set(BAQuicLib_FOUND TRUE)
    return()
endif()

string(FIND "${ba-quic-lib_DIR}" "${CMAKE_BINARY_DIR}" _ba_quic_lib_dir_idx)
if(_ba_quic_lib_dir_idx EQUAL 0)
    unset(ba-quic-lib_DIR CACHE)
endif()
unset(_ba_quic_lib_dir_idx)
find_package(ba-quic-lib QUIET CONFIG)
if(ba-quic-lib_FOUND)
    message(STATUS "[BA] ba-quic-lib: found via system package")
    set(BAQuicLib_FOUND TRUE)
    return()
endif()

message(STATUS "[BA] ba-quic-lib: system package not found, fetching via FetchContent")
include(FetchContent)
set(_token $ENV{BA_GITLAB_TOKEN_URI})
FetchContent_Declare(ba-quic-lib
    GIT_REPOSITORY "https://${_token}gitlab.bringauto.com/bring-auto/libraries/quic-lib.git"
    GIT_TAG        v0.1.1
    GIT_SHALLOW    TRUE
    OVERRIDE_FIND_PACKAGE)
# ba-quic-lib's own CMakeLists.txt declares `option(BRINGAUTO_TESTS ...)` and
# `option(BRINGAUTO_INSTALL ...)` with the same names as this repo's. Since FetchContent pulls it in
# via add_subdirectory, an already-cached value from *this* repo would otherwise leak into it:
#  - BRINGAUTO_TESTS=ON would also build ba-quic-lib's own test suite (gtest, test certs, etc.).
#  - BRINGAUTO_INSTALL=ON (set by this repo's CMDEF packaging) would run ba-quic-lib's
#    install(EXPORT ba-quic-lib-targets), which fails at generate time: the exported ba-quic-lib
#    target links msquic PUBLIC, but msquic (also a FetchContent subdir target) is in no export set,
#    and CMake forbids exporting a target whose public dependency isn't exported too. We link
#    ba-quic-lib statically in-tree and never consume its install/export, so force both OFF.
# Force both cache entries OFF for the duration of this add_subdirectory, then restore them
# afterwards. A plain-variable shadow would be simpler but silently depends on CMP0077=NEW being in
# effect *inside ba-quic-lib's own directory scope*, which is decided by ba-quic-lib's own
# cmake_minimum_required(), not anything we control or can reliably assert from here. FORCE-writing
# the cache is more verbose but works unconditionally.
set(_ba_quic_lib_saved_tests_flag "${BRINGAUTO_TESTS}")
set(_ba_quic_lib_saved_install_flag "${BRINGAUTO_INSTALL}")
set(BRINGAUTO_TESTS OFF CACHE BOOL "" FORCE)
set(BRINGAUTO_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ba-quic-lib)
set(BRINGAUTO_TESTS "${_ba_quic_lib_saved_tests_flag}" CACHE BOOL "Enable tests" FORCE)
set(BRINGAUTO_INSTALL "${_ba_quic_lib_saved_install_flag}" CACHE BOOL "Enable install" FORCE)
unset(_ba_quic_lib_saved_tests_flag)
unset(_ba_quic_lib_saved_install_flag)
set(BAQuicLib_FOUND TRUE)
