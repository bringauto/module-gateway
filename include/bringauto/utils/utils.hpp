#pragma once

#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>
#include <InternalProtocol.pb.h>
#include <memory_management.h>
#include <device_management.h>

#include <memory>



namespace bringauto::utils {

void initLogger(const std::string &logPath, bool verbose);

void initStatusAggregators(std::shared_ptr <bringauto::structures::GlobalContext> &context);

void loadLibraries(std::map<unsigned int, std::shared_ptr<bringauto::modules::ModuleManagerLibraryHandler>> &modules,
				   const std::map<int, std::string> &libPaths);

std::vector <std::string> splitString(const std::string &input, char delimiter);

std::string getId(const ::device_identification &device);

void initBuffer(struct buffer &buffer, const std::string &data);

void deallocateDeviceId(struct device_identification &device);

}