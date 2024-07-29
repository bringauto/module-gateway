#include <ErrorAggregatorTests.hpp>
#include <testing_utils/DeviceIdentificationHelper.h>
#include <fleet_protocol/module_gateway/error_codes.h>


namespace bam = bringauto::modules;
namespace external_client = bringauto::external_client;


bam::Buffer ErrorAggregatorTests::init_status_buffer() {
    size_t size = strlen(BUTTON_UNPRESSED);
    bam::Buffer buffer = libHandler_->constructBuffer(size);
	std::memcpy(buffer.getStructBuffer().data, BUTTON_UNPRESSED, size);
	return buffer;
}

void ErrorAggregatorTests::SetUp(){
    libHandler_ = std::make_shared<bam::ModuleManagerLibraryHandler>();
	libHandler_->loadLibrary(PATH_TO_MODULE);
    errorAggregator_.init_error_aggregator(libHandler_);
}

void ErrorAggregatorTests::TearDown(){
	errorAggregator_.destroy_error_aggregator();
}

TEST_F(ErrorAggregatorTests, init_error_aggregator_ok) {
	external_client::ErrorAggregator errorAggregatorTest;
    auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandler>();
	int ret = errorAggregatorTest.init_error_aggregator(libHandler);
	EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, destroy_error_aggregator_ok) {
    external_client::ErrorAggregator errorAggregatorTest;
    auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandler>();
    errorAggregatorTest.init_error_aggregator(libHandler);
    int ret = errorAggregatorTest.destroy_error_aggregator();
    EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_device_not_supported) {
    bam::Buffer status = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_SUPPORTED);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_ok) {
    bam::Buffer status = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_not_registered) {
    auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandler>();
	libHandler->loadLibrary(PATH_TO_MODULE);
    bam::Buffer buffer = libHandler->constructBuffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator_.get_last_status(buffer, deviceId);
    EXPECT_EQ(ret, DEVICE_NOT_REGISTERED);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_ok) {
    bam::Buffer status = init_status_buffer();
    auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE, SUPPORTED_DEVICE_TYPE, "button", "name", 10);
    int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
    EXPECT_EQ(ret, OK);
    auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandler>();
	libHandler->loadLibrary(PATH_TO_MODULE);
    bam::Buffer buffer = libHandler->constructBuffer();
    ret = errorAggregator_.get_last_status(buffer, deviceId);
    EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, get_module_number) {
	int ret = errorAggregator_.get_module_number();
	EXPECT_EQ(ret, 1000);
}

TEST_F(ErrorAggregatorTests, is_device_type_supported) {
    int ret = errorAggregator_.is_device_type_supported(SUPPORTED_DEVICE_TYPE);
    EXPECT_EQ(ret, OK);
}
