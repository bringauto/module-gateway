#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>
#include <bringauto/settings/Constants.hpp>

#include <bringauto/async_function_execution/AsyncFunctionExecutor.hpp>
#include <bringauto/fleet_protocol/cxx/AsyncModuleFunctionDefinitions.hpp>
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

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	void loadLibrary(const std::filesystem::path &path) override;

	int getModuleNumber() override;

	int isDeviceTypeSupported(unsigned int device_type) override;

	int	sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) override;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type) override;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type) override;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int	aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
						  unsigned int device_type) override;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateFirstCommand(Buffer &default_command, unsigned int device_type) override;

	int statusDataValid(const Buffer &status, unsigned int device_type) override;

	int commandDataValid(const Buffer &command, unsigned int device_type) override;

	/**
	 * @brief Constructs a buffer with the given size
	 * 
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0) override;

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const;

	void deallocate(struct buffer *buffer) const;

	/**
	 * @brief Constructs a buffer with the same raw c buffer as provided
	 * 
	 * @param buffer c buffer to be used
	 * @return a new Buffer object
	 */
	Buffer constructBufferByTakeOwnership(struct ::buffer& buffer);

	std::function<void(struct buffer *)> deallocate_ {};

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

	fleet_protocol::cxx::ModuleFunctionExecutor aeronClient {
		async_function_execution::Config {
			.isProducer = true,
			.defaultTimeout = settings::AeronClientConstants::aeron_client_default_timeout,
		},
		fleet_protocol::cxx::moduleFunctionList
	};
};

}
