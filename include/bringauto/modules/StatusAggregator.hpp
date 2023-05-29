#pragma once

#include <map>
#include <queue>
#include <string>

#include <InternalProtocol.pb.h>
#include <command_manager.h>
#include <module_manager.h>



namespace bringauto::modules {

using fnc_void = int (*)();
using fnc_int = int (*)(int device);
using fnc_buff_int = int (*)(struct buffer buffer, int device_type);
using fnc_buffPtr_int = int (*)(struct buffer *gen_buffer, int device_type);
using fnc_buffPtr_buff_buff_int = int (*)(struct buffer *gen_buffer, const struct buffer curr_buffer,
										  const struct buffer new_buffer, int device_type);
using fnc_buffPtr_buff_buff_buff_int = int (*)(struct buffer *gen_buffer, const struct buffer new_buffer,
											   const struct buffer curr_stat_buffer,
											   const struct buffer curr_comm_stat, int device_type);

class StatusAggregator {
public:
	int init_status_aggregator(const char *path);

	int destroy_status_aggregator(); /// module specific

	int clear_all_devices();

	int clear_device(const struct ::device_identification device);

	int remove_device(const struct ::device_identification device);

	int get_aggregated_status(struct ::buffer *generated_status, const struct ::device_identification device);

	int get_unique_devices(struct ::buffer *unique_devices_buffer);

	int force_aggregation_on_device(const struct ::device_identification device);

	int is_device_valid(const struct ::device_identification device);

	int get_module_number();

	int update_command(const struct ::buffer command, const struct ::device_identification device);

	int get_command(const struct ::buffer status, const struct ::device_identification device,
					struct ::buffer *command);

	// this function does not exists in status_aggregator but it needed
	int is_device_type_supported(int device_type);

	struct device_state {
		std::queue<InternalProtocol::DeviceStatus> receivedMessages;
		std::queue<InternalProtocol::DeviceStatus> aggregatedMessages;
		InternalProtocol::DeviceCommand command;

		device_state() = default;

		device_state(const buffer buffer,
					 const InternalProtocol::Device device) { // this constructor is just for testing
			command.set_allocated_commanddata(static_cast<std::string *>(buffer.data));
			command.set_allocated_device(new InternalProtocol::Device(device));
		}
	};

private:
	std::string getId(const ::device_identification &device);

	int add_status_to_aggregator(const struct ::buffer status, const struct ::device_identification device);

	void *module{ nullptr };

	std::map<std::string, device_state> devices {};

	fnc_void getModuleNumber { nullptr };
	fnc_int isDeviceTypeSupported { nullptr };
	fnc_buffPtr_int generateFirstCommand { nullptr };
	fnc_buff_int statusDataValid { nullptr };
	fnc_buff_int commandDataValid { nullptr };
	fnc_buffPtr_buff_buff_int aggregateStatus { nullptr };
	fnc_buffPtr_buff_buff_buff_int generateCommand { nullptr };
};

}