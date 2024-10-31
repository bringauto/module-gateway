#pragma once

#include <bringauto/logging/LoggerVerbosity.hpp>

#include <filesystem>


namespace bringauto::structures {

struct ConsoleSettings {
    /// Flag to enable console logging
    bool use {};
    /// Console log level
    logging::LoggerVerbosity level {};
};

struct FileSettings {
    /// Flag to enable file logging
    bool use {};
    /// File log level
    logging::LoggerVerbosity level {};
    /// Path to the log file directory
    std::filesystem::path path {};
};

struct LoggingSettings {
    /// Console logging settings
    ConsoleSettings console {};
    /// File logging settings
    FileSettings file {};
};

}
