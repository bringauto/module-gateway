#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>
#include <bringauto/settings/Constants.hpp>

#include <bringauto/async_function_execution/AsyncFunctionExecutor.hpp>
#include <bringauto/fleet_protocol/async_function_execution_definitions/AsyncModuleFunctionDefinitions.hpp>
#include <boost/process.hpp>

#include <mutex>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandlerAsync : public IModuleManagerLibraryHandler {
public:
	explicit ModuleManagerLibraryHandlerAsync(const std::filesystem::path &moduleBinaryPath, const int moduleNumber);

	~ModuleManagerLibraryHandlerAsync() override;

	ModuleManagerLibraryHandlerAsync(const ModuleManagerLibraryHandlerAsync &) = delete;
	ModuleManagerLibraryHandlerAsync(ModuleManagerLibraryHandlerAsync &&) = delete;
	ModuleManagerLibraryHandlerAsync &operator=(const ModuleManagerLibraryHandlerAsync &) = delete;
	ModuleManagerLibraryHandlerAsync &operator=(ModuleManagerLibraryHandlerAsync &&) = delete;

	/**
	 * @brief Spawns the module binary process and waits for it to be ready via Aeron IPC.
	 */
	void loadLibrary(const std::filesystem::path &path) override;

	int getModuleNumber() override;

	int isDeviceTypeSupported(unsigned int device_type) override;

	int sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) override;

	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type) override;

	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type) override;

	int aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
					   unsigned int device_type) override;

	int generateFirstCommand(Buffer &default_command, unsigned int device_type) override;

	int statusDataValid(const Buffer &status, unsigned int device_type) override;

	int commandDataValid(const Buffer &command, unsigned int device_type) override;

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const override;

	void deallocate(struct buffer *buffer) const;

	/// Path to the module binary
	std::filesystem::path moduleBinaryPath_ {};
	/// Process of the module binary
	boost::process::child moduleBinaryProcess_ {};

	/// TODO find a way to not need this
	std::mutex getModuleNumberMutex_ {};
	std::mutex isDeviceTypeSupportedMutex_ {};
	std::mutex sendStatusConditionMutex_ {};
	std::mutex generateCommandMutex_ {};
	std::mutex aggregateStatusMutex_ {};
	std::mutex aggregateErrorMutex_ {};
	std::mutex generateFirstCommandMutex_ {};
	std::mutex statusDataValidMutex_ {};
	std::mutex commandDataValidMutex_ {};

	fleet_protocol::async_function_execution_definitions::ModuleFunctionExecutor aeronClient {
		async_function_execution::Config {
			.isProducer = true,
			.defaultTimeout = settings::AeronClientConstants::aeron_client_default_timeout,
		},
		fleet_protocol::async_function_execution_definitions::moduleFunctionList
	};
};

}
