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
	 * @brief Spawns the module binary process and waits for it to be ready
	 */
	void loadLibrary(const std::filesystem::path &path) override;

	/**
	 * @brief Returns the module number via an asynchronous Aeron call
	 */
	int getModuleNumber() override;

	/**
	 * @brief Checks device type support via an asynchronous Aeron call
	 */
	int isDeviceTypeSupported(unsigned int device_type) override;

	/**
	 * @brief Evaluates the status aggregation condition via an asynchronous Aeron call
	 */
	int sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) override;

	/**
	 * @brief Generates a command via an asynchronous Aeron call
	 */
	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type) override;

	/**
	 * @brief Aggregates status via an asynchronous Aeron call
	 */
	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type) override;

	/**
	 * @brief Aggregates error via an asynchronous Aeron call
	 */
	int aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
					   unsigned int device_type) override;

	/**
	 * @brief Generates the first command via an asynchronous Aeron call
	 */
	int generateFirstCommand(Buffer &default_command, unsigned int device_type) override;

	/**
	 * @brief Validates status data via an asynchronous Aeron call
	 */
	int statusDataValid(const Buffer &status, unsigned int device_type) override;

	/**
	 * @brief Validates command data via an asynchronous Aeron call
	 */
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
