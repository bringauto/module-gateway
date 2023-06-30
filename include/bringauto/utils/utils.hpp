#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <InternalProtocol.pb.h>

#include <memory>



namespace bringauto::utils {

void initLogger(const std::string &logPath, bool verbose);

void initStatusAggregators(std::shared_ptr <bringauto::structures::GlobalContext> &context);

void loadLibraries(std::map<unsigned int, std::shared_ptr<bringauto::modules::ModuleManagerLibraryHandler>> &modules,
				   const std::map<int, std::string> &libPaths);

/**
 * @brief Creates device_identification from string
 *
 * @param device string
 * @return ::device_identification
 */
device_identification mapToDeviceId(const std::string &device);

std::vector <std::string> splitString(const std::string &input, char delimiter);

}