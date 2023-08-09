#pragma once

#include <memory_management.h>
#include <device_management.h>

#include <memory>
#include <vector>



namespace bringauto::utils {

void initLogger(const std::string &logPath, bool verbose);

std::vector <std::string> splitString(const std::string &input, char delimiter);

std::string getId(const ::device_identification &device);

void initBuffer(struct buffer &buffer, const std::string &data);

void deallocateDeviceId(struct ::device_identification &device);

}