#pragma once

#include <filesystem>
#include <dlfcn.h>



namespace bringauto::modules::library_loader {

/**
 * @brief Load a shared library from the given path
 *
 * @param path path to the shared library file
 * @return handle to the loaded library, or nullptr on failure
 */
void* load(const std::filesystem::path& path);

}
