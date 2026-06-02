#include <bringauto/modules/library_loader.hpp>

#include <dlfcn.h>



namespace bringauto::modules::library_loader {

void* load(const std::filesystem::path& path) { // NOSONAR cpp:S5008 - dlopen C API requires void*
	return ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

}
