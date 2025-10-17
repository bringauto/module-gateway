#pragma once

#include <bringauto/external_client/connection/ExternalConnection.hpp>
#include <bringauto/common_utils/ProtobufUtils.hpp>
#include <bringauto/settings/LoggerId.hpp>
#include <testing_utils/CommunicationMock.hpp>

#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>
#include <gtest/gtest.h>



class ExternalConnectionTests: public ::testing::Test {
protected:
	void SetUp() override {
		context_ = std::make_shared<bringauto::structures::GlobalContext>();
		bringauto::settings::Settings settings = {
			.port = 0,
			.modulePaths = {{ MODULE, PATH_TO_MODULE }},
			.externalConnectionSettingsList = {{
				bringauto::structures::ProtocolType::INVALID, // protocolType
				{},                                           // protocolSettings
				"",                                           // serverIp
				0,                                            // port
				{ MODULE }                                    // module numbers
			}}
		};
		context_->settings = std::make_shared<bringauto::settings::Settings>(settings);

		moduleLibrary_ = std::make_shared<bringauto::structures::ModuleLibrary>();
		try {
			moduleLibrary_->loadLibraries(context_->settings->modulePaths);
			moduleLibrary_->initStatusAggregators(context_);
		} catch(std::exception &e) {
			std::cerr << "[ERROR] Error occurred during module initialization: " << e.what() << std::endl;
			return;
		}

		fromExternalQueue_ = std::make_shared<bringauto::structures::AtomicQueue<InternalProtocol::DeviceCommand >>();
		reconnectQueue_ = std::make_shared<bringauto::structures::AtomicQueue<bringauto::structures::ReconnectQueueItem >>();

		externalConnection_ = std::make_unique<bringauto::external_client::connection::ExternalConnection>(
			context_,
			*moduleLibrary_,
			context_->settings->externalConnectionSettingsList[0],
			fromExternalQueue_,
			reconnectQueue_
		);

		communicationChannel_ = std::make_shared<testing_utils::CommunicationMock>(context_->settings->externalConnectionSettingsList[0]);
		externalConnection_->init(communicationChannel_);
		::device_identification device {
			.module = MODULE,
			.device_type = BUTTON_DEVICE_TYPE,
			.device_role = create_buffer("role").getStructBuffer(),
			.device_name = create_buffer("name").getStructBuffer(),
			.priority = 0
		};
		connectedDevices_.emplace_back(bringauto::structures::DeviceIdentification(device));
		externalConnection_->fillErrorAggregator(bringauto::common_utils::ProtobufUtils::createDeviceStatus(
			connectedDevices_[0], create_buffer("status")));
	};

	void TearDown() override {
		externalConnection_->deinitializeConnection(true);
		context_.reset();
		moduleLibrary_.reset();
		fromExternalQueue_.reset();
		reconnectQueue_.reset();
		communicationChannel_.reset();
		connectedDevices_.clear();
		externalConnection_.reset();
	};

	static void SetUpTestSuite() {
		bringauto::settings::Logger::destroy();
		bringauto::settings::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::logging::LoggerSettings settings {
			"ExternalConnectionTests",
			bringauto::settings::toLoggerVerbosity(MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY)
		};
		bringauto::settings::Logger::init(settings);
	};

	bringauto::modules::Buffer create_buffer(const char* string) {
		const auto size = std::string(string).size();
		auto buffer = moduleLibrary_->moduleLibraryHandlers[MODULE]->constructBuffer(size);
		std::memcpy(buffer.getStructBuffer().data, string, size);
		return buffer;
	}

	void expectFailedConnectionSequence() {
		ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::NOT_INITIALIZED);
		ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), -1);
		ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::NOT_CONNECTED);
	}

	std::shared_ptr<bringauto::structures::GlobalContext> context_ {};
	std::shared_ptr<bringauto::structures::ModuleLibrary> moduleLibrary_ {};
	std::shared_ptr<bringauto::structures::AtomicQueue<InternalProtocol::DeviceCommand>> fromExternalQueue_ {};
	std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::ReconnectQueueItem>> reconnectQueue_ {};
	std::shared_ptr<testing_utils::CommunicationMock> communicationChannel_ {};
	std::vector<bringauto::structures::DeviceIdentification> connectedDevices_ {};
	std::unique_ptr<bringauto::external_client::connection::ExternalConnection> externalConnection_ {};
#ifdef DEBUG
	static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-sharedd.so" };
#else
	static constexpr const char* PATH_TO_MODULE { "./test/lib/example-module/libexample-module-gateway-shared.so" };
#endif
	static constexpr const int MODULE = 1000;
	static constexpr const int BUTTON_DEVICE_TYPE = 0;
	static constexpr const int EXPECTED_SESSION_ID_LENGTH = 8;
};
