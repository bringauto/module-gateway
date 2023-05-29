SET(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)

BA_PACKAGE_LIBRARY(protobuf      v4.21.12)
BA_PACKAGE_LIBRARY(nlohmann-json v3.10.5 PLATFORM_STRING_MODE any_machine NO_DEBUG ON)
BA_PACKAGE_LIBRARY(cxxopts       v3.0.5  PLATFORM_STRING_MODE any_machine NO_DEBUG ON)
BA_PACKAGE_LIBRARY(boost      v1.78.0)
BA_PACKAGE_LIBRARY(ba-logger     v1.2.0)

IF (BRINGAUTO_TESTS)
    BA_PACKAGE_LIBRARY(gtest         v1.12.1)
ENDIF ()