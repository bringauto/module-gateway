#pragma once

#include <fleet_protocol/common_headers/device_management.h>
#include <fleet_protocol/common_headers/general_error_codes.h>

#include <memory>
#include <cstring>
#include <functional>



namespace bringauto::modules {

/**
 * @brief Buffer structure used to simplify buffer management.
 */
struct Buffer final {

	friend class ModuleManagerLibraryHandler;

	Buffer() = default;
	Buffer(const Buffer& buff) = default;
	Buffer(Buffer&& buff) = default;
	~Buffer() = default;

	Buffer& operator=(const Buffer& buff) = default;
	Buffer& operator=(Buffer&& buff) = default;

	/**
	 * @brief Get a raw pointer to the buffer.
	 *
	 * @return pointer to a buffer
	 */
	[[nodiscard]] inline struct ::buffer getStructBuffer() const {
		if(buffer_ == nullptr) [[unlikely]] {
			throw BufferNotAllocated { "Buffer not allocated - it cannot be used as raw C struct" };
		}
		return raw_buffer_;
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
		buffer_ = std::shared_ptr<void>(raw_buffer_.data, [deallocate, size](void *data){
			struct ::buffer buff {
				.data = data,
				.size_in_bytes = size
			};
			deallocate(&buff);
		});
	}

	struct ::buffer raw_buffer_ {};
	std::shared_ptr<void> buffer_ {nullptr};
};

}
