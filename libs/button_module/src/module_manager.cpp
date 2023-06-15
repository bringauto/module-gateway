#include <string>
#include <cstring>

#include <device_management.h>
#include <module_manager.h>
#include <memory_management.h>



enum DeviceType {
	BUTTON = 0
};

void pressed_true(struct buffer *buffer) {
	const char *command = "{\"lit_up\": true}";
	size_t size = strlen(command) + 1;
	allocate(buffer, size);
	strcpy(static_cast<char *>(buffer->data), command);
}

void pressed_false(struct buffer *buffer) {
	const char *command = "{\"lit_up\": false}";
	size_t size = strlen(command) + 1;
	allocate(buffer, size);
	strcpy(static_cast<char *>(buffer->data), command);
}

int
send_status_condition(const struct buffer current_status, const struct buffer new_status, unsigned int device_type) {
	auto curr_data = static_cast<char *>(current_status.data);
	auto new_data = static_cast<char *>(new_status.data);
	if(strcmp(curr_data, new_data) == 0) {
		return OK;
	}
	return NOT_OK;
}

int generate_command(struct buffer *generated_command, const struct buffer new_status,
					 const struct buffer current_status, const struct buffer current_command,
					 unsigned int device_type) {
	const char *data = static_cast<char *>(new_status.data);
	if(strcmp(data, "{\"pressed\": false}") == 0) {
		pressed_true(generated_command);
	} else if(strcmp(data, "{\"pressed\": true}") == 0) {
		pressed_false(generated_command);
	}
	return OK;
}

int aggregate_status(struct buffer *aggregated_status, const struct buffer current_status,
					 const struct buffer new_status, unsigned int device_type) {
	allocate(aggregated_status, new_status.size_in_bytes);
	strncpy(static_cast<char *>(aggregated_status->data), static_cast<char *>(new_status.data),
			new_status.size_in_bytes - 1);
	return OK;
}

int aggregate_error(struct buffer *error_message, const struct buffer current_error_message, const struct buffer status,
					unsigned int device_type) {
	return OK;
}

int generate_first_command(struct buffer *default_command, unsigned int device_type) {
	if(device_type == DeviceType::BUTTON) {
		pressed_false(default_command);
	} else {
		return NOT_OK;
	}
	return OK;
}

int status_data_valid(const struct buffer status, unsigned int device_type) {
	const char *data = static_cast<char *>(status.data);
	if(data == nullptr) {
		return NOT_OK;
	}
	switch(device_type) {
		case BUTTON:
			if(strcmp(data, "{\"pressed\": false}") == 0 || strcmp(data, "{\"pressed\": true}") == 0) {
				return OK;
			}
			break;
		default:
			break;
	}

	return NOT_OK;
}

int command_data_valid(const struct buffer command, unsigned int device_type) {
	const char *data = static_cast<char *>(command.data);
	if(data == nullptr) {
		return NOT_OK;
	}
	switch(device_type) {
		case BUTTON:
			if(strcmp(data, "{\"lit_up\": false}") == 0 || strcmp(data, "{\"lit_up\": true}") == 0) {
				return OK;
			}
			break;
		default:
			break;
	}
	return NOT_OK;
}

int get_module_number() { return 2; }

int is_device_type_supported(unsigned int device_type) {
	switch(device_type) {
		case DeviceType::BUTTON:
			return OK;
		default:
			break;
	}
	return NOT_OK;
}