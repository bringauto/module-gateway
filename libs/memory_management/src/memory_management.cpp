#include <new>

#include <memory_management.h>
#include <general_error_codes.h>


int allocate(struct buffer *buffer_pointer, size_t size_in_bytes){
    try{
        buffer_pointer->data = new char[size_in_bytes]();
    } catch(std::bad_alloc&){
        return NOT_OK;
    }
    buffer_pointer->size_in_bytes = size_in_bytes;
    return OK;
}

void deallocate(struct buffer *buffer_pointer){
	// if (buffer_pointer->data != nullptr) {
		delete[] static_cast<char *>(buffer_pointer->data);
		buffer_pointer->data = nullptr;
	// }
    buffer_pointer->size_in_bytes = 0;
}