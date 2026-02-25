#include <bringauto/modules/IModuleManagerLibraryHandler.hpp>

#include <fleet_protocol/common_headers/general_error_codes.h>



namespace bringauto::modules {

Buffer IModuleManagerLibraryHandler::constructBuffer(std::size_t size) {
	if (size == 0) {
		return Buffer {};
	}
	struct ::buffer buff {};
	buff.size_in_bytes = size;
	if(allocate(&buff, size) != OK) {
		throw std::bad_alloc {};
	}
	return { buff, deallocate_ };
}

Buffer IModuleManagerLibraryHandler::constructBufferByTakeOwnership(struct ::buffer &buffer) {
	if (buffer.data == nullptr) {
		throw Buffer::BufferNotAllocated { "Buffer not allocated - cannot take ownership" };
	}
	return { buffer, deallocate_ };
}

}
