#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <bringauto/logging/Logger.hpp>
#include <general_error_codes.h>

#include <dlfcn.h>



namespace bringauto::modules {

template <typename T>
struct FunctionTypeDeducer;

template<typename R, typename ...Args>
struct FunctionTypeDeducer<std::function<R(Args...)>>{
	using fncptr = R (*)(Args...);
};

using log = bringauto::logging::Logger;

int ModuleManagerLibraryHandler::loadLibrary(const std::filesystem::path& path) {
	try {
		module_ = dlopen(path.c_str(), RTLD_LAZY | RTLD_DEEPBIND);
		if (module_ == nullptr) {
			return NOT_OK;
		}
		isDeviceTypeSupported = reinterpret_cast<FunctionTypeDeducer<decltype(isDeviceTypeSupported)>::fncptr>(dlsym(
				module_, "is_device_type_supported"));
		getModuleNumber = reinterpret_cast<FunctionTypeDeducer<decltype(getModuleNumber)>::fncptr>(dlsym(module_,
																										 "get_module_number"));
		generateFirstCommand = reinterpret_cast<FunctionTypeDeducer<decltype(generateFirstCommand)>::fncptr>(dlsym(
				module_, "generate_first_command"));
		statusDataValid = reinterpret_cast<FunctionTypeDeducer<decltype(statusDataValid)>::fncptr>(dlsym(module_,
																										 "status_data_valid"));
		commandDataValid = reinterpret_cast<FunctionTypeDeducer<decltype(commandDataValid)>::fncptr>(dlsym(module_,
																										   "command_data_valid"));
		sendStatusCondition = reinterpret_cast<FunctionTypeDeducer<decltype(sendStatusCondition)>::fncptr>(dlsym(
				module_, "send_status_condition"));
		aggregateStatus = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateStatus)>::fncptr>(dlsym(module_,
																										 "aggregate_status"));
		generateCommand = reinterpret_cast<FunctionTypeDeducer<decltype(generateCommand)>::fncptr>(dlsym(module_,
																										 "generate_command"));
		aggregateError = reinterpret_cast<FunctionTypeDeducer<decltype(aggregateError)>::fncptr>(dlsym(module_,
																									   "aggregate_error"));
		return OK;
	} catch (const std::exception& e) {
		log::logError("Error occurred during loading library: \"{}\": {}", path.string(), e.what());
		return NOT_OK;
	}
}

ModuleManagerLibraryHandler::~ModuleManagerLibraryHandler() {
    if(module_ != nullptr){
    	dlclose(module_);
        module_ == nullptr;
    }
}
}