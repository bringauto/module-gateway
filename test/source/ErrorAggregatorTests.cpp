#include <ErrorAggregatorTests.hpp>
#include <testing_utils/DeviceIdentificationHelper.h>
#include <bringauto/common_utils/MemoryUtils.hpp>
#include <fleet_protocol/module_gateway/error_codes.h>

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
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_SUPPORTED);
    deallocate(&status);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_ok) {
    struct buffer status = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
    deallocate(&status);

}

TEST_F(ErrorAggregatorTests, get_last_status_device_not_registered) {
    struct buffer buffer;
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator.get_last_status(&buffer, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_REGISTERED);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_ok) {
    struct buffer status = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
    struct buffer buffer {};
    ret = errorAggregator.get_last_status(&buffer, deviceId);
    EXPECT_EQ(ret, OK);
    deallocate(&status);
}

TEST_F(ErrorAggregatorTests, get_module_number) {
	int ret = errorAggregator.get_module_number();
	EXPECT_EQ(ret, 1000);
}

TEST_F(ErrorAggregatorTests, is_device_type_supported) {
    int ret = errorAggregator.is_device_type_supported(SUPPORTED_DEVICE_TYPE);
    EXPECT_EQ(ret, OK);
}
