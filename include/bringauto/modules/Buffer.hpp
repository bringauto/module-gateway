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
	~Buffer() = default;

	Buffer& operator=(const Buffer& buff) = default;

	/**
	 * @brief Get a raw pointer to the buffer.
	 *
	 * @return pointer to a buffer
	 */
	inline struct ::buffer getStructBuffer() const { return *buffer_; }

private:

	/**
	 * @brief Construct Buffer object with given buffer and deallocate function.
	 *
	 * @param buff buffer to be assigned to buffer_
	 * @param deallocate function to be called when buffer_ is destroyed
	 */
	Buffer(const struct ::buffer& buff, std::function<void(struct ::buffer*)> deallocate) {
		buffer_ = std::shared_ptr<struct ::buffer>(new struct ::buffer, [deallocate](struct ::buffer *buff){
			deallocate(buff);
			delete buff;
		});
		*buffer_ = buff;
	}

	std::shared_ptr<struct ::buffer> buffer_ {};
};

}
