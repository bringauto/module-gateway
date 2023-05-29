#include <device_management.h>
#include <module_manager.h>

#include <fstream>
// std::ofstream myfile;
// myfile.open ("example2.txt");
// myfile << device.device_name;
// myfile.close();

enum DeviceType { BUTTON = 0 };

int send_status_condition(const struct buffer current_status, const struct buffer new_status, int device_type) {
    return OK;
}

int generate_command(struct buffer *generated_command, const struct buffer new_status,
                     const struct buffer current_status, const struct buffer current_command, int device_type) {
    std::string *data = static_cast<std::string *>(new_status.data);
    if (*data == "{\"pressed\": false}") {
        generated_command->data = static_cast<void *>(new std::string("{\"lit_up\": true}"));
    } else if (*data == "{\"pressed\": true}") {
        generated_command->data = static_cast<void *>(new std::string("{\"lit_up\": false}"));
    }
    return OK;
}

int aggregate_status(struct buffer *aggregated_status, const struct buffer current_status,
                     const struct buffer new_status, int device_type) {
    aggregated_status->data = new_status.data;
    aggregated_status->size_in_bytes = new_status.size_in_bytes;
    return OK;
}

int aggregate_error(struct buffer *error_message, const struct buffer current_error_message, const struct buffer status,
                    int device_type) {
    return OK;
}

int generate_first_command(struct buffer *default_command, int device_type) {
    switch (device_type) {
        case DeviceType::BUTTON:
            default_command->data = static_cast<void *>(new std::string("{\"lit_up\": false}"));
        default:
            return NOT_OK;
    }
    return OK;
}

int get_module_number() { return 2; }

int status_data_valid(const struct buffer status, int device_type) {
    std::string *data = static_cast<std::string *>(status.data);
    if (*data == "{\"pressed\": false}" || *data == "{\"pressed\": true}")
        return OK;
    return NOT_OK;
}

int command_data_valid(const struct buffer command, int device_type) {
    std::string *data = static_cast<std::string *>(command.data);
    if (*data == std::string("{\"lit_up\": false}") || *data == std::string("{\"lit_up\": true}"))
        return OK;
    return NOT_OK;
}

int is_device_type_supported(int device_type) {
    switch (device_type) {
        case DeviceType::BUTTON:
            return OK;
        default:
            break;
    }
    return NOT_OK;
}