#pragma once

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

  friend class ModuleManagerLibraryHandler;

  Buffer() = default;
  Buffer(const Buffer& buff) = default;
  ~Buffer() = default;

  Buffer& operator=(const Buffer& buff) = default;

  inline struct ::buffer getStructBuffer() const { return *buffer_; }

private:

    Buffer(const struct ::buffer& buff, std::function<void(struct ::buffer*)> deallocate) {
      buffer_ = std::shared_ptr<struct ::buffer>(new struct ::buffer, [deallocate](struct ::buffer *buff){ deallocate(buff); delete buff; });
      *buffer_ = buff;
    }

    std::shared_ptr<struct ::buffer> buffer_ {};
};

}
