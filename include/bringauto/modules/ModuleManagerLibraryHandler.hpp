#pragma once

#include <bringauto/modules/Buffer.hpp>
#include <bringauto/aeron_communication/AeronClient.hpp>

#include <fleet_protocol/common_headers/memory_management.h>

#include <functional>
#include <filesystem>



namespace bringauto::modules {

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandler {
public:
	ModuleManagerLibraryHandler() = default;
	ModuleManagerLibraryHandler(std::shared_ptr<aeron_communication::AeronClient> &aeronClient):
		aeronClient_(aeronClient) {};

	~ModuleManagerLibraryHandler();

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	void loadLibrary(const std::filesystem::path &path);

	int getModuleNumber() const;

	int isDeviceTypeSupported(unsigned int device_type) const;

	int	sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) const;

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateCommand(Buffer &generated_command, const Buffer &new_status,
						const Buffer &current_status, const Buffer &current_command,
						unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int aggregateStatus(Buffer &aggregated_status, const Buffer &current_status,
						const Buffer &new_status, unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int	aggregateError(Buffer &error_message, const Buffer &current_error_message, const Buffer &status,
						  unsigned int device_type);

	/**
	 * @short After executing the respective module function, an error might be thrown when allocating the buffer.
	 * 
	 * @see fleet-protocol/lib/module_maintainer/module_gateway/include/module_manager.h
	 */
	int generateFirstCommand(Buffer &default_command, unsigned int device_type);

	int statusDataValid(const Buffer &status, unsigned int device_type) const;

	int commandDataValid(const Buffer &command, unsigned int device_type) const;

	/**
	 * @brief Constructs a buffer with the given size
	 * 
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0);

private:

	int allocate(struct buffer *buffer_pointer, size_t size_in_bytes) const;

	void deallocate(struct buffer *buffer) const;

	void *checkFunction(const char *functionName) const;

	/**
	 * @brief Constructs a buffer with the same raw c buffer as provided
	 * 
	 * @param buffer c buffer to be used
	 * @return a new Buffer object
	 */
	Buffer constructBufferByTakeOwnership(struct ::buffer& buffer);

	/**
	 * @brief Constructs a message for AeronClient to send to the module
	 * 
	 * @param buffers vector of raw c buffers to be included in the message
	 * @param deviceType type of the device for which the message is constructed
	 * @return a string message to be sent
	 */
	std::string constructAeronMessage(const std::vector<struct ::buffer *> &buffers, int deviceType) const;

	/**
	 * @brief Parses the response from AeronClient
	 * 
	 * @param raw_buffer raw buffer to be filled with the response data
	 * @param response response string from AeronClient
	 * @return status code of the response
	 */
	int parseAeronResponse(struct ::buffer &raw_buffer, std::string_view response) const;

	void *module_ {};

	std::function<int()> getModuleNumber_ {};
	std::function<int(unsigned int)> isDeviceTypeSupported_ {};
	std::function<int(struct buffer *, unsigned int)> generateFirstCommand_ {};
	std::function<int(const struct buffer, unsigned int)> statusDataValid_ {};
	std::function<int(const struct buffer, unsigned int)> commandDataValid_ {};
	std::function<int(struct buffer, struct buffer, unsigned int)> sendStatusCondition_ {};
	std::function<int(struct buffer *, struct buffer, struct buffer, unsigned int)> aggregateStatus_ {};
	std::function<int(struct buffer *error_message,
					  const struct buffer current_error_message,
					  const struct buffer status,
					  unsigned int device_type)> aggregateError_ {};
	std::function<int(struct buffer *, struct buffer, struct buffer, struct buffer, unsigned int)> generateCommand_ {};
	std::function<int(struct buffer *, size_t)> allocate_ {};
	std::function<void(struct buffer *)> deallocate_ {};
	std::shared_ptr<aeron_communication::AeronClient> aeronClient_ {nullptr};
};

}
