#include <ErrorAggregatorTests.hpp>
#include <testing_utils/DeviceIdentificationHelper.h>
#include <bringauto/modules/ModuleManagerLibraryHandlerLocal.hpp>

#include <fleet_protocol/module_gateway/error_codes.h>


namespace bam = bringauto::modules;
namespace external_client = bringauto::external_client;


bam::Buffer ErrorAggregatorTests::init_status_buffer() {
	const auto size = std::string(BUTTON_UNPRESSED).size();
	auto buffer = libHandler_->constructBuffer(size);
	std::memcpy(buffer.getStructBuffer().data, BUTTON_UNPRESSED, size);
	return buffer;
}

void ErrorAggregatorTests::SetUp(){
	libHandler_ = std::make_shared<bam::ModuleManagerLibraryHandlerLocal>();
	libHandler_->loadLibrary(PATH_TO_MODULE);
	errorAggregator_.init_error_aggregator(libHandler_);
}

void ErrorAggregatorTests::TearDown(){
	errorAggregator_.destroy_error_aggregator();
}

TEST_F(ErrorAggregatorTests, init_error_aggregator_ok) {
	external_client::ErrorAggregator errorAggregatorTest {};
	const auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandlerLocal>();
	const int ret = errorAggregatorTest.init_error_aggregator(libHandler);
	EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, destroy_error_aggregator_ok) {
	external_client::ErrorAggregator errorAggregatorTest {};
	const auto libHandler = std::make_shared<bam::ModuleManagerLibraryHandlerLocal>();
	errorAggregatorTest.init_error_aggregator(libHandler);
	const int ret = errorAggregatorTest.destroy_error_aggregator();
	EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_device_not_supported) {
	const auto status = init_status_buffer();
	const auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE,
		UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
	const int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
	EXPECT_EQ(ret, DEVICE_NOT_SUPPORTED);
}

TEST_F(ErrorAggregatorTests, add_status_to_error_aggregator_ok) {
	const auto status = init_status_buffer();
	const auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE,
		SUPPORTED_DEVICE_TYPE, "button", "name", 10);
	const int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
	EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_not_registered) {
	bringauto::modules::Buffer buffer {};
	const auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE,
		UNSUPPORTED_DEVICE_TYPE, "button", "name", 10);
	const int ret = errorAggregator_.get_last_status(buffer, deviceId);
	EXPECT_EQ(ret, DEVICE_NOT_REGISTERED);
}

TEST_F(ErrorAggregatorTests, get_last_status_device_ok) {
	const auto status = init_status_buffer();
	const auto deviceId = testing_utils::DeviceIdentificationHelper::createDeviceIdentification(MODULE,
		SUPPORTED_DEVICE_TYPE, "button", "name", 10);
	int ret = errorAggregator_.add_status_to_error_aggregator(status, deviceId);
	EXPECT_EQ(ret, OK);
	bringauto::modules::Buffer buffer {};
	ret = errorAggregator_.get_last_status(buffer, deviceId);
	EXPECT_EQ(ret, OK);
}

TEST_F(ErrorAggregatorTests, get_module_number) {
	const int ret = errorAggregator_.get_module_number();
	EXPECT_EQ(ret, 1000);
}

TEST_F(ErrorAggregatorTests, is_device_type_supported) {
	const int ret = errorAggregator_.is_device_type_supported(SUPPORTED_DEVICE_TYPE);
	EXPECT_EQ(ret, OK);
}
