#include <StatusAggregatorTests.hpp>
#include <testing_utils/DeviceIdentificationHelper.h>
#include <bringauto/common_utils/MemoryUtils.hpp>


struct buffer StatusAggregatorTests::init_status_buffer(){
	struct buffer buffer{};
	size_t size = strlen(BUTTON_UNPRESSED);
	allocate(&buffer, size);
	std::memcpy(buffer.data, BUTTON_UNPRESSED, buffer.size_in_bytes);
	return buffer;
}

struct buffer StatusAggregatorTests::init_command_buffer(){
	struct buffer buffer{};
	size_t size = strlen(LIT_DOWN);
	allocate(&buffer, size);
	std::memcpy(buffer.data, LIT_DOWN, buffer.size_in_bytes);
	return buffer;
}

void StatusAggregatorTests::add_status_to_aggregator(){
	struct buffer status_buffer = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
	deallocate(&status_buffer);
}

void StatusAggregatorTests::remove_device_from_status_aggregator(){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->remove_device(deviceId);
	EXPECT_TRUE(ret == OK);
}

void StatusAggregatorTests::SetUp(){
	context = std::make_shared<bringauto::structures::GlobalContext>();
	auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	libHandler->loadLibrary(PATH_TO_MODULE);
	statusAggregator = std::make_unique<bringauto::modules::StatusAggregator>(context, libHandler);
	statusAggregator->init_status_aggregator();
}

void StatusAggregatorTests::TearDown(){
	statusAggregator->destroy_status_aggregator();
}

TEST_F(StatusAggregatorTests, init_status_aggregator_ok) {
	bringauto::modules::StatusAggregator statusAggregatorTest;
	int ret = statusAggregatorTest.init_status_aggregator();
	EXPECT_TRUE(ret == OK);
	statusAggregatorTest.destroy_status_aggregator();
}

TEST_F(StatusAggregatorTests, init_status_aggregator_bad_path) {
	auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	EXPECT_THROW(libHandler->loadLibrary(WRONG_PATH_TO_MODULE), std::runtime_error);
}

TEST_F(StatusAggregatorTests, destroy_status_aggregator_ok) {
	bringauto::modules::StatusAggregator statusAggregatorTest;
	int ret = statusAggregatorTest.init_status_aggregator();
	EXPECT_TRUE(ret == OK);
	ret = statusAggregatorTest.destroy_status_aggregator();
	EXPECT_TRUE(ret == OK);
}

TEST_F(StatusAggregatorTests, is_device_type_supported_ok){
	int ret = statusAggregator->is_device_type_supported(SUPPORTED_DEVICE_TYPE);
	EXPECT_TRUE(ret == OK);
}

TEST_F(StatusAggregatorTests, is_device_type_supported_not_ok){
	int ret = statusAggregator->is_device_type_supported(UNSUPPORTED_DEVICE_TYPE);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_device_type_not_supported){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	struct buffer status_buffer{};
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_status_register_device){
	add_status_to_aggregator();
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_without_aggregation){
	add_status_to_aggregator();
	struct buffer status_buffer{};
	size_t size = strlen(BUTTON_PRESSED) + 1;
	allocate(&status_buffer, size);
	strcpy(static_cast<char *>(status_buffer.data), BUTTON_PRESSED);
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 2);
	ret = statusAggregator->add_status_to_aggregator(init_status_buffer(), deviceId);
	EXPECT_TRUE(ret == 3);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_with_aggregation){
	add_status_to_aggregator();
	struct buffer status_buffer = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
	ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_aggregated_status_device_not_registered){
	struct buffer status_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->get_aggregated_status(&status_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, get_aggregated_status_no_message){
	add_status_to_aggregator();
	struct buffer status_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	statusAggregator->get_aggregated_status(&status_buffer, deviceId);
	int ret = statusAggregator->get_aggregated_status(&status_buffer, deviceId);
	EXPECT_TRUE(ret == NO_MESSAGE_AVAILABLE);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_unique_devices_empty){
	struct buffer unique_devices{};
	int ret = statusAggregator->get_unique_devices(&unique_devices);
	EXPECT_TRUE(ret == 0);
}

TEST_F(StatusAggregatorTests, get_unique_devices_one){
	add_status_to_aggregator();
	struct buffer unique_devices{};
	int ret = statusAggregator->get_unique_devices(&unique_devices);
	EXPECT_TRUE(ret == 1);
	device_identification *devicesPointer = static_cast<device_identification *>(unique_devices.data);
	struct device_identification deviceId {
		.module = devicesPointer[0].module,
		.device_type = devicesPointer[0].device_type,
		.device_role = devicesPointer[0].device_role,
		.device_name = devicesPointer[0].device_name
	};
	ASSERT_EQ(MODULE, deviceId.module);
	ASSERT_EQ(SUPPORTED_DEVICE_TYPE, deviceId.device_type);
	deallocate(&deviceId.device_role);
	deallocate(&deviceId.device_name);
	deallocate(&unique_devices);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_unique_devices_two){
	add_status_to_aggregator();
	struct buffer status_buffer = init_status_buffer();
    auto deviceId2 = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE_2, DEVICE_NAME_2, 0);
    int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId2);
	EXPECT_TRUE(ret == 1);
	struct buffer unique_devices{};
	ret = statusAggregator->get_unique_devices(&unique_devices);
	EXPECT_TRUE(ret == 2);
	device_identification *devicesPointer = static_cast<device_identification *>(unique_devices.data);
	struct device_identification deviceId {
		.module = devicesPointer[0].module,
		.device_type = devicesPointer[0].device_type,
		.device_role = devicesPointer[0].device_role,
		.device_name = devicesPointer[0].device_name,
        .priority = devicesPointer[0].priority
	};
    ASSERT_EQ(MODULE, deviceId.module);
    ASSERT_EQ(SUPPORTED_DEVICE_TYPE, deviceId.device_type);
    auto deviceName = std::string((const char*)deviceId.device_name.data, deviceId.device_name.size_in_bytes);
    auto deviceRole = std::string((const char*)deviceId.device_role.data, deviceId.device_role.size_in_bytes);
    ASSERT_STREQ(DEVICE_ROLE_2, deviceRole.c_str());
    ASSERT_STREQ(DEVICE_NAME_2, deviceName.c_str());
    ASSERT_EQ(0, deviceId.priority);
	deallocate(&deviceId.device_role);
	deallocate(&deviceId.device_name);
    // TODO will the order of devices be always the same?

	deviceId = {
		.module = devicesPointer[1].module,
		.device_type = devicesPointer[1].device_type,
		.device_role = devicesPointer[1].device_role,
		.device_name = devicesPointer[1].device_name,
        .priority = devicesPointer[1].priority
	};
    ASSERT_EQ(MODULE, deviceId.module);
    ASSERT_EQ(SUPPORTED_DEVICE_TYPE, deviceId.device_type);
    deviceName = std::string((const char*)deviceId.device_name.data, deviceId.device_name.size_in_bytes);
    deviceRole = std::string((const char*)deviceId.device_role.data, deviceId.device_role.size_in_bytes);
    ASSERT_STREQ(DEVICE_ROLE, deviceRole.c_str());
    ASSERT_STREQ(DEVICE_NAME, deviceName.c_str());
    ASSERT_EQ(10, deviceId.priority);
	deallocate(&deviceId.device_role);
	deallocate(&deviceId.device_name);
	deallocate(&status_buffer);
	deallocate(&unique_devices);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, force_aggregation_on_device_not_registered){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->force_aggregation_on_device(deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, is_device_valid_not){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->is_device_valid(deviceId);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, is_device_valid_ok){
	add_status_to_aggregator();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->is_device_valid(deviceId);
	EXPECT_TRUE(ret == OK);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, update_command_device_not_supported){
	struct buffer command_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, update_command_device_not_registered){
	struct buffer command_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, update_command_device_command_invalid){
	add_status_to_aggregator();
	struct buffer command_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == COMMAND_INVALID);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, update_command_device_ok){
	add_status_to_aggregator();
	struct buffer command_buffer = init_command_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == OK);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_command_device_not_supported){
	struct buffer command_buffer{};
	struct buffer status_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->get_command(status_buffer, deviceId, &command_buffer);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, get_command_device_status_invalid){
	struct buffer command_buffer{};
	struct buffer status_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->get_command(status_buffer, deviceId, &command_buffer);
	EXPECT_TRUE(ret == STATUS_INVALID);
}

TEST_F(StatusAggregatorTests, get_command_ok){
	struct buffer status_buffer = init_status_buffer();
	struct buffer command_buffer{};
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator->get_command(status_buffer, deviceId, &command_buffer);
	EXPECT_TRUE(ret == OK);
	std::string command {static_cast<char *>(command_buffer.data), command_buffer.size_in_bytes};
	ASSERT_STREQ(LIT_DOWN, command.c_str());
	deallocate(&command_buffer);
	deallocate(&status_buffer);
}