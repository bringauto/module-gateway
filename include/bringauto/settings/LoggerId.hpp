#pragma once

#include <libbringauto_logger/bringauto/logging/Logger.hpp>

namespace bringauto::settings {

constexpr bringauto::logging::LoggerId logId = {.id = "ModuleGateway"};
using Logger = bringauto::logging::Logger<logId, bringauto::logging::LoggerImpl>;

}
