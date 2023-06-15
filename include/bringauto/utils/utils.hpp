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
 * @brief Creates device_identification from protobuf device
 *
 * @param device internal protocol device
 * @return ::device_identification
 */
::device_identification mapToDeviceId(const InternalProtocol::Device &device);

}