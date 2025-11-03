#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>

#include <bringauto/async_function_execution/AsyncFunctionExecutor.hpp>
#include <boost/process.hpp>

#include <mutex>




namespace bringauto::modules {

struct ConvertibleBufferReturn final {
	int returnCode {};
	struct ::buffer buffer {};
	ConvertibleBufferReturn() = default;
	ConvertibleBufferReturn(int code, struct ::buffer buff) : returnCode(code), buffer(buff) {}

	std::span<const uint8_t> serialize() const {
		size_t total_size = sizeof(int) + buffer.size_in_bytes;
		uint8_t* data = new uint8_t[total_size];
		std::memcpy(data, &returnCode, sizeof(int));
		std::memcpy(data + sizeof(int), buffer.data, buffer.size_in_bytes);
		return {data, total_size};
	}
	void deserialize(std::span<const uint8_t> bytes) {
		auto size = bytes.size();
		if (size < sizeof(int)) return;
		std::memcpy(&returnCode, bytes.data(), sizeof(int));
		size -= sizeof(int);
		allocate(&buffer, size);
		std::memcpy(buffer.data, bytes.data() + sizeof(int), size);
		buffer.size_in_bytes = size;
	}
};

struct ConvertibleBuffer final {
	struct ::buffer buffer {};
	ConvertibleBuffer() = default;
	ConvertibleBuffer(struct ::buffer buff) : buffer(buff) {}
	
	std::span<const uint8_t> serialize() const {
		return std::span {reinterpret_cast<const uint8_t *>(buffer.data), buffer.size_in_bytes};
	}
	void deserialize(std::span<const uint8_t> bytes) {
		buffer.data = const_cast<uint8_t *>(bytes.data());
		buffer.size_in_bytes = bytes.size();
	}
};

inline static const async_function_execution::FunctionDefinition getModuleNumberAsync {
	async_function_execution::FunctionId { 0 },
	async_function_execution::Return { int {} },
	async_function_execution::Arguments {}
};

inline static const async_function_execution::FunctionDefinition isDeviceTypeSupportedAsync {
	async_function_execution::FunctionId { 1 },
	async_function_execution::Return { int {} },
	async_function_execution::Arguments { uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition sendStatusConditionAsync {
	async_function_execution::FunctionId { 2 },
	async_function_execution::Return { int {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, ConvertibleBuffer {}, uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition generateCommandAsync {
	async_function_execution::FunctionId { 3 },
	async_function_execution::Return { ConvertibleBufferReturn {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, ConvertibleBuffer {}, uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition aggregateStatusAsync {
	async_function_execution::FunctionId { 4 },
	async_function_execution::Return { ConvertibleBufferReturn {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, ConvertibleBuffer {}, uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition aggregateErrorAsync {
	async_function_execution::FunctionId { 5 },
	async_function_execution::Return { ConvertibleBufferReturn {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, ConvertibleBuffer {}, uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition generateFirstCommandAsync {
	async_function_execution::FunctionId { 6 },
	async_function_execution::Return { ConvertibleBufferReturn {} },
	async_function_execution::Arguments { uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition statusDataValidAsync {
	async_function_execution::FunctionId { 7 },
	async_function_execution::Return { int {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, uint32_t {} }
};

inline static const async_function_execution::FunctionDefinition commandDataValidAsync {
	async_function_execution::FunctionId { 8 },
	async_function_execution::Return { int {} },
	async_function_execution::Arguments { ConvertibleBuffer {}, uint32_t {} }
};

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandlerAsync : public IModuleManagerLibraryHandler {
public:
	explicit ModuleManagerLibraryHandlerAsync(const std::filesystem::path &moduleBinaryPath);

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
};

}
