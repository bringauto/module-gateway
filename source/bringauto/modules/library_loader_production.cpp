#include <bringauto/modules/library_loader.hpp>

#define _GNU_SOURCE
#include <dlfcn.h>



namespace bringauto::modules::library_loader {

void* load(const std::filesystem::path& path) { // NOSONAR cpp:S5008 - dlmopen C API requires void*
	return ::dlmopen(LM_ID_NEWLM, path.c_str(), RTLD_LAZY);
}

}
