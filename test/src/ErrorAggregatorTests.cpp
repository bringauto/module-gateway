#include <ErrorAggregatorTests.hpp>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <mg_error_codes.h>

const struct device_identification ErrorAggregatorTests::init_device_id(unsigned int type, const char* deviceRole, const char* deviceName){
	size_t deviceRoleSize = strlen(deviceRole);
	struct buffer deviceRoleBuff{};
	allocate(&deviceRoleBuff, deviceRoleSize);
	std::memcpy(deviceRoleBuff.data, deviceRole, deviceRoleSize);

	size_t deviceNameSize = strlen(deviceName);
	struct buffer deviceNameBuff{};
	allocate(&deviceNameBuff, deviceNameSize);
	std::memcpy(deviceNameBuff.data, deviceName, deviceNameSize);

	const struct ::device_identification device_id {
		.module=2,
		.device_type=type,
		.device_role=deviceRoleBuff,
		.device_name=deviceNameBuff,
		.priority=10
	};
	return device_id;
}

struct buffer ErrorAggregatorTests::init_status_buffer(){
	struct buffer buffer{};
	size_t size = strlen(BUTTON_UNPRESSED);
	allocate(&buffer, size);
	std::memcpy(buffer.data, BUTTON_UNPRESSED, buffer.size_in_bytes);
	return buffer;
}

void ErrorAggregatorTests::SetUp(){
    auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	libHandler->loadLibrary(PATH_TO_MODULE);
    errorAggregator.init_error_aggregator(libHandler);
}

void ErrorAggregatorTests::TearDown(){
	errorAggregator.destroy_error_aggregator();
}

TEST_F(ErrorAggregatorTests, init_error_aggregator_ok) {
	bringauto::external_client::ErrorAggregator errorAggregatorTest;
    auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
	int ret = errorAggregatorTest.init_error_aggregator(libHandler);
	EXPECT_EQ(ret, OK);
	errorAggregatorTest.destroy_error_aggregator();
}

TEST_F(ErrorAggregatorTests, destroy_error_aggregator_ok) {
    bringauto::external_client::ErrorAggregator errorAggregatorTest;
    auto libHandler = std::make_shared<bringauto::modules::ModuleManagerLibraryHandler>();
    errorAggregatorTest.init_error_aggregator(libHandler);
    int ret = errorAggregatorTest.destroy_error_aggregator();
    EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_device_not_supported) {
    struct buffer status = init_status_buffer();
    struct device_identification deviceId = init_device_id(UNSUPPORTED_DEVICE_TYPE, "button", "name");
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_SUPPORTED);
	bringauto::common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_ok) {
    struct buffer status = init_status_buffer();
    struct device_identification deviceId = init_device_id(SUPPORTED_DEVICE_TYPE, "button", "name");
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
	bringauto::common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_not_registered) {
    struct buffer buffer;
    struct device_identification deviceId = init_device_id(UNSUPPORTED_DEVICE_TYPE, "button", "name");
    int ret = errorAggregator.get_last_status(&buffer, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_REGISTERED);
	bringauto::common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_ok) {
    struct buffer status = init_status_buffer();
    struct device_identification deviceId = init_device_id(SUPPORTED_DEVICE_TYPE, "button", "name");
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
    struct buffer buffer;
    ret = errorAggregator.get_last_status(&buffer, deviceId);
    EXPECT_EQ(ret, OK);
	bringauto::common_utils::MemoryUtils::deallocateDeviceId(deviceId);
}

TEST_F(ErrorAggregatorTests, get_module_number) {
	int ret = errorAggregator.get_module_number();
	EXPECT_EQ(ret, 1000);
}

TEST_F(ErrorAggregatorTests, is_device_type_supported) {
    int ret = errorAggregator.is_device_type_supported(SUPPORTED_DEVICE_TYPE);
    EXPECT_EQ(ret, OK);
}
