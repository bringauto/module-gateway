#include <bringauto/modules/library_loader.hpp>



namespace bringauto::modules::library_loader {

void* load(const std::filesystem::path& path) {
	return ::dlmopen(LM_ID_NEWLM, path.c_str(), RTLD_LAZY);
}

}
