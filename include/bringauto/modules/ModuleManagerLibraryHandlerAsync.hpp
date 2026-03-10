#pragma once

#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>

#include <boost/process.hpp>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>




namespace bringauto::modules {

struct ConvertibleBufferReturn final {
	int returnCode {};
	std::vector<uint8_t> data {};

	ConvertibleBufferReturn() = default;
	ConvertibleBufferReturn(int code, struct ::buffer buff)
		: returnCode(code),
		  data(static_cast<uint8_t *>(buff.data),
			   static_cast<uint8_t *>(buff.data) + buff.size_in_bytes) {}

	std::span<const uint8_t> serialize() const {
		serialized_.resize(sizeof(int) + data.size());
		std::memcpy(serialized_.data(), &returnCode, sizeof(int));
		if(!data.empty()) {
			std::memcpy(serialized_.data() + sizeof(int), data.data(), data.size());
		}
		return serialized_;
	}
	void deserialize(std::span<const uint8_t> bytes) {
		if(bytes.size() < sizeof(int)) { return; }
		std::memcpy(&returnCode, bytes.data(), sizeof(int));
		auto payload = bytes.subspan(sizeof(int));
		data.assign(payload.begin(), payload.end());
	}

private:
	mutable std::vector<uint8_t> serialized_ {};
};

struct ConvertibleBuffer final {
	struct ::buffer buffer {};
	ConvertibleBuffer() = default;
	explicit ConvertibleBuffer(struct ::buffer buff) : buffer(buff) {}

	std::span<const uint8_t> serialize() const {
		return std::span {static_cast<const uint8_t *>(buffer.data), buffer.size_in_bytes};
	}
	void deserialize(std::span<const uint8_t> bytes) {
		data_.assign(bytes.begin(), bytes.end());
		buffer.data = data_.data();
		buffer.size_in_bytes = data_.size();
	}

private:
	std::vector<uint8_t> data_ {};
};

/**
 * @brief Exception thrown when the module binary fails to start or become ready
 */
class ModuleBinaryException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * @brief Class used to load and handle library created by module maintainer
 */
class ModuleManagerLibraryHandlerAsync : public IModuleManagerLibraryHandler {
public:
	explicit ModuleManagerLibraryHandlerAsync(const std::filesystem::path &moduleBinaryPath);

	~ModuleManagerLibraryHandlerAsync() override;

	ModuleManagerLibraryHandlerAsync(const ModuleManagerLibraryHandlerAsync &) = delete;
	ModuleManagerLibraryHandlerAsync(ModuleManagerLibraryHandlerAsync &&) = delete;
	ModuleManagerLibraryHandlerAsync &operator=(const ModuleManagerLibraryHandlerAsync &) = delete;
	ModuleManagerLibraryHandlerAsync &operator=(ModuleManagerLibraryHandlerAsync &&) = delete;

	/**
	 * @brief Load library created by a module maintainer
	 *
	 * @param path path to the library
	 */
	void loadLibrary(const std::filesystem::path &path) override;

	int getModuleNumber() const override;

	int isDeviceTypeSupported(unsigned int device_type) override;

	int	sendStatusCondition(const Buffer &current_status, const Buffer &new_status, unsigned int device_type) const override;

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

	int statusDataValid(const Buffer &status, unsigned int device_type) const override;

	int commandDataValid(const Buffer &command, unsigned int device_type) const override;

	/**
	 * @brief Constructs a buffer with the given size
	 *
	 * @param size size of the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::size_t size = 0) override;

private:

	/**
	 * @brief Constructs a buffer containing a copy of the given data
	 *
	 * @param data span of bytes to copy into the buffer
	 * @return a new Buffer object
	 */
	Buffer constructBuffer(std::span<const uint8_t> data);

	/// Path to the module binary
	std::filesystem::path moduleBinaryPath_ {};
	/// Process of the module binary
	boost::process::child moduleBinaryProcess_ {};
	/// TODO find a way to not need this
	std::mutex tmpMutex_ {};
};

}
