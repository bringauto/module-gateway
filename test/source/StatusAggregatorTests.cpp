#include <StatusAggregatorTests.hpp>
#include <testing_utils/DeviceIdentificationHelper.h>
#include <bringauto/common_utils/MemoryUtils.hpp>


bringauto::modules::Buffer StatusAggregatorTests::init_status_buffer(){
	size_t size = strlen(BUTTON_UNPRESSED);
	bringauto::modules::Buffer buffer = libHandler_->constructBufferByAllocate(size);
	std::memcpy(buffer.getStructBuffer().data, BUTTON_UNPRESSED, size);
	return buffer;
}

bringauto::modules::Buffer StatusAggregatorTests::init_command_buffer(){
	size_t size = strlen(LIT_DOWN);
	bringauto::modules::Buffer buffer = libHandler_->constructBufferByAllocate(size);
	std::memcpy(buffer.getStructBuffer().data, LIT_DOWN, size);
	return buffer;
}

bringauto::modules::Buffer StatusAggregatorTests::init_empty_buffer(){
	bringauto::modules::Buffer buffer = libHandler_->constructBufferByAllocate();
	return buffer;
}

void StatusAggregatorTests::add_status_to_aggregator(){
	bringauto::modules::Buffer status_buffer = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
}

void StatusAggregatorTests::remove_device_from_status_aggregator(){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->remove_device(deviceId);
	EXPECT_TRUE(ret == OK);
}

void StatusAggregatorTests::SetUp(){
	context_ = std::make_shared<bringauto::structures::GlobalContext>();
	libHandler_ = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	libHandler_->loadLibrary(PATH_TO_MODULE);
	statusAggregator_ = std::make_unique<bringauto::modules::StatusAggregator>(context_, libHandler_);
	statusAggregator_->init_status_aggregator();
}

void StatusAggregatorTests::TearDown(){
	statusAggregator_->destroy_status_aggregator();
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
	int ret = statusAggregator_->is_device_type_supported(SUPPORTED_DEVICE_TYPE);
	EXPECT_TRUE(ret == OK);
}

TEST_F(StatusAggregatorTests, is_device_type_supported_not_ok){
	int ret = statusAggregator_->is_device_type_supported(UNSUPPORTED_DEVICE_TYPE);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_device_type_not_supported){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	bringauto::modules::Buffer status_buffer = init_empty_buffer();
	int ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_status_register_device){
	add_status_to_aggregator();
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_without_aggregation){
	auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	libHandler->loadLibrary(PATH_TO_MODULE);
	add_status_to_aggregator();
	size_t size = strlen(BUTTON_PRESSED) + 1;
	bringauto::modules::Buffer status_buffer = libHandler->constructBufferByAllocate(size);
	strcpy(static_cast<char *>(status_buffer.getStructBuffer().data), BUTTON_PRESSED);
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 2);
    status_buffer = init_status_buffer();
	ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 3);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_with_aggregation){
	add_status_to_aggregator();
	bringauto::modules::Buffer status_buffer = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
	ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == 1);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_aggregated_status_device_not_registered){
	bringauto::modules::Buffer status_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->get_aggregated_status(status_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, get_aggregated_status_no_message){
	add_status_to_aggregator();
	bringauto::modules::Buffer status_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	statusAggregator_->get_aggregated_status(status_buffer, deviceId);
	int ret = statusAggregator_->get_aggregated_status(status_buffer, deviceId);
	EXPECT_TRUE(ret == NO_MESSAGE_AVAILABLE);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_unique_devices_empty){
	bringauto::modules::Buffer unique_devices = init_empty_buffer();
	int ret = statusAggregator_->get_unique_devices(unique_devices);
	EXPECT_TRUE(ret == 0);
}

TEST_F(StatusAggregatorTests, get_unique_devices_one){
	add_status_to_aggregator();
	bringauto::modules::Buffer unique_devices = init_empty_buffer();
	int ret = statusAggregator_->get_unique_devices(unique_devices);
	EXPECT_TRUE(ret == 1);
	device_identification *devicesPointer = static_cast<device_identification *>(unique_devices.getStructBuffer().data);
	struct device_identification deviceId {
		.module = devicesPointer[0].module,
		.device_type = devicesPointer[0].device_type,
		.device_role = devicesPointer[0].device_role,
		.device_name = devicesPointer[0].device_name,
        .priority = devicesPointer[0].priority
	};
	ASSERT_EQ(MODULE, deviceId.module);
	ASSERT_EQ(SUPPORTED_DEVICE_TYPE, deviceId.device_type);
	deallocate(&deviceId.device_role);
	deallocate(&deviceId.device_name);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_unique_devices_two){
	add_status_to_aggregator();
	bringauto::modules::Buffer status_buffer = init_status_buffer();
	auto deviceId1 = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
    auto deviceId2 = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE_2, DEVICE_NAME_2, 0);
    int ret = statusAggregator_->add_status_to_aggregator(status_buffer, deviceId2);
	EXPECT_TRUE(ret == 1);
	bringauto::modules::Buffer unique_devices = init_empty_buffer();
	ret = statusAggregator_->get_unique_devices(unique_devices);
	EXPECT_TRUE(ret == 2);
	device_identification *devicesPointer = static_cast<device_identification *>(unique_devices.getStructBuffer().data);

	bool deviceId1Found = false;
	bool deviceId2Found = false;
	for(int i = 0; i < 2; ++i) {
		bringauto::structures::DeviceIdentification currDeviceId(devicesPointer[i]);
		if(currDeviceId.getPriority() == 10) {
			ASSERT_EQ(currDeviceId, deviceId1);
			deviceId1Found = true;
		} else if(currDeviceId.getPriority() == 0) {
			ASSERT_EQ(currDeviceId, deviceId2);
			deviceId2Found = true;
		} else {
			FAIL(); // None of the devices should have a priority different from 10 or 0
		}
	}
	ASSERT_TRUE(deviceId1Found);
	ASSERT_TRUE(deviceId2Found);

	deallocate(&devicesPointer[0].device_role);
	deallocate(&devicesPointer[0].device_name);
	deallocate(&devicesPointer[1].device_role);
	deallocate(&devicesPointer[1].device_name);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, force_aggregation_on_device_not_registered){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->force_aggregation_on_device(deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, is_device_valid_not){
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->is_device_valid(deviceId);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, is_device_valid_ok){
	add_status_to_aggregator();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->is_device_valid(deviceId);
	EXPECT_TRUE(ret == OK);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, update_command_device_not_supported){
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, update_command_device_not_registered){
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, update_command_device_command_invalid){
	add_status_to_aggregator();
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == COMMAND_INVALID);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, update_command_device_ok){
	add_status_to_aggregator();
	bringauto::modules::Buffer command_buffer = init_command_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->update_command(command_buffer, deviceId);
	EXPECT_TRUE(ret == OK);
	remove_device_from_status_aggregator();
}

TEST_F(StatusAggregatorTests, get_command_device_not_supported){
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
	bringauto::modules::Buffer status_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->get_command(status_buffer, deviceId, command_buffer);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, get_command_device_status_invalid){
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
	bringauto::modules::Buffer status_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	int ret = statusAggregator_->get_command(status_buffer, deviceId, command_buffer);
	EXPECT_TRUE(ret == STATUS_INVALID);
}

TEST_F(StatusAggregatorTests, get_command_ok){
	bringauto::modules::Buffer status_buffer = init_status_buffer();
	bringauto::modules::Buffer command_buffer = init_empty_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, DEVICE_ROLE, DEVICE_NAME, 10);
	statusAggregator_->add_status_to_aggregator(status_buffer, deviceId);
	int ret = statusAggregator_->get_command(status_buffer, deviceId, command_buffer);
	EXPECT_TRUE(ret == OK);
	std::string command {static_cast<char *>(command_buffer.getStructBuffer().data), command_buffer.getStructBuffer().size_in_bytes};
	ASSERT_STREQ(LIT_UP, command.c_str());
}