
#pragma once

#include <bringauto/modules/ModuleManagerLibraryHandler.hpp>

#include <fleet_protocol/common_headers/device_management.h>

#include <memory>
#include <cstring>



namespace bringauto::modules {

/**
*
*/
struct Buffer final {

  friend class ModuleManagerLibraryHandler;

  Buffer() = delete;
  ~Buffer() = default;

  inline struct ::buffer getStructBuffer() const { return *buffer_; }

private:

    Buffer(const struct ::buffer& buff, std::function<> deallocate) {
      buffer_ = std::shared_ptr<struct ::buffer>({}, deallocate);
      *buffer_ = buff;
    }

    std::shared_ptr<struct ::buffer> buffer_ {};
};

}