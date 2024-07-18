#pragma once

// #include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <fleet_protocol/common_headers/device_management.h>
#include <fleet_protocol/common_headers/general_error_codes.h>

#include <memory>
#include <cstring>
#include <functional>



namespace bringauto::modules {

/**
*
*/
struct Buffer final {

  // friend class ModuleManagerLibraryHandler;

  Buffer() = default;
  ~Buffer() = default;

  inline struct ::buffer getStructBuffer() const { return *buffer_; }
  void setStructBuffer(void* data, std::size_t size) {
    struct ::buffer buff;
		if(allocate(&buff, size) != OK) {
			throw std::runtime_error("Could not allocate memory for buffer");
		}

    std::memcpy(buff.data, data, size);
    //deallocate(buffer_.get());
    buffer_ = std::shared_ptr<struct ::buffer>(&buff, deallocate);
    //buffer_->size_in_bytes = size;
  }

private:

    Buffer(const struct ::buffer& buff, std::function<void(struct ::buffer*)> deallocate) {
      buffer_ = std::shared_ptr<struct ::buffer>({}, deallocate);
      *buffer_ = buff;
    }

    std::shared_ptr<struct ::buffer> buffer_ {};
};

}
