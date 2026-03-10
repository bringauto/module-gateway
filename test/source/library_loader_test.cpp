#include <bringauto/modules/library_loader.hpp>



namespace bringauto::modules::library_loader {

void* load(const std::filesystem::path& path) {
	return ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

}
