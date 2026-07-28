#include "QuicCommunicationTests.hpp"

#include <bringauto/settings/Constants.hpp>


namespace {
bringauto::structures::ExternalConnectionSettings makeSettings(
	std::unordered_map<std::string, std::string, bringauto::structures::TransparentStringHash,
	                   std::equal_to<>> protocolSettings
) {
	return bringauto::structures::ExternalConnectionSettings {
		.protocolType = bringauto::structures::ProtocolType::QUIC,
		.protocolSettings = std::move(protocolSettings),
		.serverIp = "127.0.0.1",
		.port = 4433,
		.modules = {}
	};
}
}


/**
 * @brief Empty protocolSettings should leave every QuicSettings field at its built-in default
 */
TEST_F(QuicCommunicationTests, BuildQuicSettingsEmptyKeepsDefaults) {
	const auto settings = buildQuicSettings(makeSettings({}));

	ASSERT_TRUE(settings.idleTimeoutMs.has_value());
	EXPECT_EQ(*settings.idleTimeoutMs, 5000);
	ASSERT_TRUE(settings.disconnectTimeoutMs.has_value());
	EXPECT_EQ(*settings.disconnectTimeoutMs, 5000);
	ASSERT_TRUE(settings.keepAliveIntervalMs.has_value());
	EXPECT_EQ(*settings.keepAliveIntervalMs, 1000);
	ASSERT_TRUE(settings.peerUnidiStreamCount.has_value());
	EXPECT_EQ(*settings.peerUnidiStreamCount, 1024);
}

/**
 * @brief A known msquic field name overrides the built-in default
 */
TEST_F(QuicCommunicationTests, BuildQuicSettingsKnownKeyOverridesDefault) {
	const auto settings = buildQuicSettings(makeSettings({
		{ "IdleTimeoutMs", "9000" }
	}));

	ASSERT_TRUE(settings.idleTimeoutMs.has_value());
	EXPECT_EQ(*settings.idleTimeoutMs, 9000);
}

/**
 * @brief An unrecognized key is ignored rather than throwing or corrupting other fields
 */
TEST_F(QuicCommunicationTests, BuildQuicSettingsUnknownKeyIgnored) {
	bringauto::quic::QuicSettings settings;
	ASSERT_NO_THROW(settings = buildQuicSettings(makeSettings({
		{ "stream-mode", "single" }
	})));

	ASSERT_TRUE(settings.idleTimeoutMs.has_value());
	EXPECT_EQ(*settings.idleTimeoutMs, 5000);
}

/**
 * @brief A non-numeric value for a numeric msquic field is a type mismatch: the field is left at
 * its default rather than throwing
 */
TEST_F(QuicCommunicationTests, BuildQuicSettingsNonNumericValueKeepsDefault) {
	const auto settings = buildQuicSettings(makeSettings({
		{ "IdleTimeoutMs", "not-a-number" }
	}));

	ASSERT_TRUE(settings.idleTimeoutMs.has_value());
	EXPECT_EQ(*settings.idleTimeoutMs, 5000);
}

/**
 * @brief Endpoint-config keys are excluded from the QUIC settings JSON, so they don't show up as
 * unrecognized msquic fields
 */
TEST_F(QuicCommunicationTests, BuildQuicSettingsExcludesEndpointConfigKeys) {
	const auto settings = buildQuicSettings(makeSettings({
		{ std::string(bringauto::settings::Constants::ALPN), "sample-alpn" },
		{ std::string(bringauto::settings::Constants::CLIENT_CERT), "/path/to/cert" },
		{ std::string(bringauto::settings::Constants::CLIENT_KEY), "/path/to/key" },
		{ std::string(bringauto::settings::Constants::CA_FILE), "/path/to/ca" }
	}));

	ASSERT_TRUE(settings.idleTimeoutMs.has_value());
	EXPECT_EQ(*settings.idleTimeoutMs, 5000);
}

/**
 * @brief buildEndpointConfig maps host/port from the settings struct and protocol settings for
 * ALPN/credential paths
 */
TEST_F(QuicCommunicationTests, BuildEndpointConfigMapsFields) {
	const auto config = buildEndpointConfig(makeSettings({
		{ std::string(bringauto::settings::Constants::ALPN), "sample-alpn" },
		{ std::string(bringauto::settings::Constants::CLIENT_CERT), "/path/to/cert" },
		{ std::string(bringauto::settings::Constants::CLIENT_KEY), "/path/to/key" },
		{ std::string(bringauto::settings::Constants::CA_FILE), "/path/to/ca" }
	}));

	EXPECT_EQ(config.host, "127.0.0.1");
	EXPECT_EQ(config.port, 4433);
	EXPECT_EQ(config.alpn, "sample-alpn");
	EXPECT_EQ(config.certPath, "/path/to/cert");
	EXPECT_EQ(config.keyPath, "/path/to/key");
	EXPECT_EQ(config.caCertsPath, "/path/to/ca");
}

/**
 * @brief Missing protocol settings fall back to the (empty) default rather than throwing
 */
TEST_F(QuicCommunicationTests, BuildEndpointConfigMissingKeysUseDefault) {
	const auto config = buildEndpointConfig(makeSettings({}));

	EXPECT_EQ(config.host, "127.0.0.1");
	EXPECT_EQ(config.port, 4433);
	EXPECT_EQ(config.alpn, "");
	EXPECT_EQ(config.certPath, "");
	EXPECT_EQ(config.keyPath, "");
	EXPECT_EQ(config.caCertsPath, "");
}
