#pragma once

#include <fleet_protocol/common_headers/device_management.h>

#include <memory>
#include <functional>



namespace bringauto::modules {

class IModuleManagerLibraryHandler;

/**
 * @brief Buffer structure used to simplify buffer management. The reason for this class is to provide
 * a way to manage buffer memory in a safe way and make it easier to pass buffers between objects.
 * All memory management is done by the library handler. A Buffer can be properly allocated
 * by the ModuleLibraryHandler function constructBuffer or by module specific functions.
 * Deallocating is done automatically when the Buffer object is destroyed.
 */
struct Buffer final {

	friend class IModuleManagerLibraryHandler;

	Buffer() = default;
	Buffer(const Buffer& buff) = default;
	Buffer(Buffer&& buff) = default;
	~Buffer() = default;

	Buffer& operator=(const Buffer& buff) = default;
	Buffer& operator=(Buffer&& buff) = default;

	/**
	 * @brief Get a valid, allocated ::buffer instance
	 *
	 * @return allocated ::buffer instance
	 */
	[[nodiscard]] inline struct ::buffer getStructBuffer() const {
		if(!isAllocated()) [[unlikely]] {
			throw BufferNotAllocated { "Buffer not allocated - it cannot be used as raw C struct" };
		}
		return raw_buffer_;
	}

	/**
	 * @brief Determine if buffer is allocated.
	 * A buffer is considered allocated if it has a non-null data pointer in its raw c buffer struct.
	 * Allocation is done by module specific functions or by constructBuffer of ModuleManagerLibraryHandler.
	 * 
	 * @return true if buffer is allocated, false otherwise
	 */
	[[nodiscard]] bool isAllocated() const {
		return raw_buffer_.data != nullptr && raw_buffer_.size_in_bytes > 0;
	}

	/**
	 * @brief Determine if buffer is empty.
	 *
	 * @return true if size of buffer is 0, false otherwise
	 */
	[[nodiscard]] bool isEmpty() const {
		return raw_buffer_.size_in_bytes == 0;
	}

private:

	/**
	 * Explicit exception for not allocated buffer.
	 * Mainly because of SAST.
	 */
	struct BufferNotAllocated: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	/**
	 * @brief Construct Buffer object with given buffer and deallocate function.
	 *
	 * @param buff buffer to be assigned to buffer_
	 * @param deallocate function to be called when buffer_ is destroyed
	 */
	Buffer(const struct ::buffer& buff, std::function<void(struct ::buffer*)> deallocate):
		raw_buffer_ { buff }
	{
		const auto size = buff.size_in_bytes;
		buffer_ = std::shared_ptr<underlying_data_type>(static_cast<underlying_data_type*>(raw_buffer_.data),
			[deallocate, size](underlying_data_type *data) {
				struct ::buffer buff {
					.data = data,
					.size_in_bytes = size
				};
				deallocate(&buff);
			}
		);
	}

	/**
	 * Underlying data type used to hold information by shared_ptr.
	 * Data type in ::buffer struct is a type void*. It is not viable
	 * to use void* in C++ --> use 1-byte data type.
	 */
	using underlying_data_type = unsigned char;

	struct ::buffer raw_buffer_ {};
	std::shared_ptr<underlying_data_type> buffer_ {nullptr};
};

}
