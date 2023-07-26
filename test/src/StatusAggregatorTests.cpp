#include <StatusAggregatorTests.hpp>


struct buffer StatusAggregatorTests::init_status_buffer(){
	struct buffer buffer{};
	size_t size = strlen(BUTTON_UNPRESSED) + 1;
	allocate(&buffer, size);
	strcpy(static_cast<char *>(buffer.data), BUTTON_UNPRESSED);
	return buffer;
}

struct buffer StatusAggregatorTests::init_command_buffer(){
	struct buffer buffer{};
	size_t size = strlen(LIT_DOWN) + 1;
	allocate(&buffer, size);
	strcpy(static_cast<char *>(buffer.data), LIT_DOWN);
	return buffer;
}

void StatusAggregatorTests::add_status_to_aggregator(){
	struct buffer status_buffer = init_status_buffer();
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == 0);
	deallocate(&status_buffer);
}

void StatusAggregatorTests::SetUp(){
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
    std::shared_ptr <bringauto::modules::ModuleManagerLibraryHandler> libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	int ret = libHandler->loadLibrary(WRONG_PATH_TO_MODULE);
	EXPECT_TRUE(ret == NOT_OK);
    bringauto::modules::StatusAggregator statusAggregatorTest{libHandler};
	ret = statusAggregatorTest.init_status_aggregator();
	EXPECT_TRUE(ret == OK);
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
	struct ::device_identification deviceId {
		.module=2,
		.device_type=100,
		.device_role=(char *)DEVICE_ROLE,
		.device_name=(char *)DEVICE_NAME,
		.priority=10
	};
	struct buffer status_buffer{};
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, deviceId);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_empty_status){
	struct buffer status_buffer{};
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_status_data_not_valid){
	struct buffer status_buffer{};
	allocate(&status_buffer, 10);
	strcpy(static_cast<char *>(status_buffer.data), "test");
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_status_register_device){
	add_status_to_aggregator();
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_without_aggregation){
	add_status_to_aggregator();
	struct buffer status_buffer{};
	size_t size = strlen(BUTTON_PRESSED) + 1;
	allocate(&status_buffer, size);
	strcpy(static_cast<char *>(status_buffer.data), BUTTON_PRESSED);
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == 1);
	ret = statusAggregator->add_status_to_aggregator(init_status_buffer(), DEVICE_ID);
	EXPECT_TRUE(ret == 2);
}

TEST_F(StatusAggregatorTests, add_status_to_aggregator_with_aggregation){
	add_status_to_aggregator();
	struct buffer status_buffer = init_status_buffer();
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == 0);
	ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == 0);
}

TEST_F(StatusAggregatorTests, get_aggregated_status_device_not_registered){
	struct buffer status_buffer{};
	int ret = statusAggregator->get_aggregated_status(&status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, get_aggregated_status_no_message){
	add_status_to_aggregator();
	struct buffer status_buffer{};
	int ret = statusAggregator->get_aggregated_status(&status_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == NO_MESSAGE_AVAILABLE);
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
    std::string devices {static_cast<char *>(unique_devices.data), unique_devices.size_in_bytes};
	ASSERT_STREQ(UNIQUE_DEVICE, devices.c_str());
	deallocate(&unique_devices);
}

TEST_F(StatusAggregatorTests, get_unique_devices_two){
	add_status_to_aggregator();
	struct buffer status_buffer = init_status_buffer();
	int ret = statusAggregator->add_status_to_aggregator(status_buffer, DEVICE_ID_2);
	EXPECT_TRUE(ret == 0);
	struct buffer unique_devices{};
	ret = statusAggregator->get_unique_devices(&unique_devices);
	EXPECT_TRUE(ret == 2);
    std::string devices {static_cast<char *>(unique_devices.data), unique_devices.size_in_bytes};
	ASSERT_STREQ(UNIQUE_DEVICES, devices.c_str());
	deallocate(&status_buffer);
	deallocate(&unique_devices);
}

TEST_F(StatusAggregatorTests, force_aggregation_on_device_not_registered){
	int ret = statusAggregator->force_aggregation_on_device(DEVICE_ID);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, is_device_valid_not){
	int ret = statusAggregator->is_device_valid(DEVICE_ID);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, is_device_valid_ok){
	add_status_to_aggregator();
	int ret = statusAggregator->is_device_valid(DEVICE_ID);
	EXPECT_TRUE(ret == OK);
}

TEST_F(StatusAggregatorTests, if_device_valid_after_remove){
	add_status_to_aggregator();
	int ret = statusAggregator->is_device_valid(DEVICE_ID);
	EXPECT_TRUE(ret == OK);
	statusAggregator->remove_device(DEVICE_ID);
	ret = statusAggregator->is_device_valid(DEVICE_ID);
	EXPECT_TRUE(ret == NOT_OK);
}

TEST_F(StatusAggregatorTests, update_command_device_not_supported){
	struct buffer command_buffer{};
	int ret = statusAggregator->update_command(command_buffer, DEVICE_ID_UNSUPPORTED_TYPE);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, update_command_device_not_registered){
	struct buffer command_buffer{};
	int ret = statusAggregator->update_command(command_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == DEVICE_NOT_REGISTERED);
}

TEST_F(StatusAggregatorTests, update_command_device_command_invalid){
	add_status_to_aggregator();
	struct buffer command_buffer{};
	int ret = statusAggregator->update_command(command_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == COMMAND_INVALID);
}

TEST_F(StatusAggregatorTests, update_command_device_ok){
	add_status_to_aggregator();
	struct buffer command_buffer = init_command_buffer();
	int ret = statusAggregator->update_command(command_buffer, DEVICE_ID);
	EXPECT_TRUE(ret == OK);
}

TEST_F(StatusAggregatorTests, get_command_device_not_supported){
	struct buffer command_buffer{};
	struct buffer status_buffer{};
	int ret = statusAggregator->get_command(status_buffer, DEVICE_ID_UNSUPPORTED_TYPE, &command_buffer);
	EXPECT_TRUE(ret == DEVICE_NOT_SUPPORTED);
}

TEST_F(StatusAggregatorTests, get_command_device_status_invalid){
	struct buffer command_buffer{};
	struct buffer status_buffer{};
	int ret = statusAggregator->get_command(status_buffer, DEVICE_ID, &command_buffer);
	EXPECT_TRUE(ret == STATUS_INVALID);
}

TEST_F(StatusAggregatorTests, get_command_ok){
	struct buffer status_buffer = init_status_buffer();
	struct buffer command_buffer{};
	int ret = statusAggregator->get_command(status_buffer, DEVICE_ID, &command_buffer);
	EXPECT_TRUE(ret == OK);
    std::string command {static_cast<char *>(command_buffer.data), command_buffer.size_in_bytes};
	ASSERT_STREQ(LIT_UP, command.c_str());
	deallocate(&command_buffer);
	deallocate(&status_buffer);
}