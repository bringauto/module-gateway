#include <module_manager.h>
#include <device_management.h>

#include <fstream>

int send_status_condition(const struct buffer current_status, const struct buffer new_status, int device_type){
    return 0;
}

int generate_command(struct buffer *generated_command, const struct buffer new_status, const struct buffer current_status, const struct buffer current_command, int device_type){
    return 0;
}

int aggregate_status(struct buffer *aggregated_status, const struct buffer current_status, const struct buffer new_status, int device_type){
    return 0;
}

int aggregate_error(struct buffer *error_message, const struct buffer current_error_message, const struct buffer status, int device_type){
    return 0;
}

int generate_first_command(struct buffer *default_command, int device_type){
    return 0;
}

int get_module_number(){
    return 1;
}

int status_data_valid(const struct buffer status, int device_type){
    return 0;
}

int command_data_valid(const struct buffer command, int device_type){
    return 0;
}

int is_device_type_supported(int device_type){
    return 0;
}