#pragma once

#include <bringauto/external_client/connection/communication/QuicCommunication.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>
#include <gtest/gtest.h>



class QuicCommunicationTests: public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		bringauto::settings::Logger::destroy();
		bringauto::settings::Logger::addSink<bringauto::logging::ConsoleSink>();
		bringauto::settings::Logger::init("QuicCommunicationTests");
	};

	static bringauto::quic::QuicSettings buildQuicSettings(
		const bringauto::structures::ExternalConnectionSettings &settings
	) {
		return bringauto::external_client::connection::communication::QuicCommunication::buildQuicSettings(settings);
	}

	static bringauto::quic::QuicEndpointConfig buildEndpointConfig(
		const bringauto::structures::ExternalConnectionSettings &settings
	) {
		return bringauto::external_client::connection::communication::QuicCommunication::buildEndpointConfig(settings);
	}
};
