#pragma once

#include <filesystem>
#include <dlfcn.h>



namespace bringauto::modules::library_loader {

void* load(const std::filesystem::path& path);

}
