#include <bringauto/settings/QuicSettingsParser.hpp>
#include <bringauto/structures/ExternalConnectionSettings.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <nlohmann/json.hpp>


namespace bringauto::settings {
	QUIC_SETTINGS QuicSettingsParser::parse(
		const structures::ExternalConnectionSettings &settings
	) {
		QUIC_SETTINGS quic{};

		if (auto value = getOptionalUint(settings, "IdleTimeoutMs")) {
			settings::Logger::logDebug("[quic] [settings] IdleTimeoutMs settings loaded");
			quic.IdleTimeoutMs = *value;
			quic.IsSet.IdleTimeoutMs = TRUE;
		}

		if (auto value = getOptionalUint(settings, "HandshakeIdleTimeoutMs")) {
			settings::Logger::logDebug("[quic] [settings] HandshakeIdleTimeoutMs settings loaded");
			quic.HandshakeIdleTimeoutMs = *value;
			quic.IsSet.HandshakeIdleTimeoutMs = TRUE;
		}

		if (auto value = getOptionalUint(settings, "DisconnectTimeoutMs")) {
			settings::Logger::logDebug("[quic] [settings] DisconnectTimeoutMs settings loaded");
			quic.DisconnectTimeoutMs = *value;
			quic.IsSet.DisconnectTimeoutMs = TRUE;
		}

		if (auto value = getOptionalUint(settings, "PeerUnidiStreamCount")) {
			settings::Logger::logDebug("[quic] [settings] PeerUnidiStreamCount settings loaded");
			quic.PeerUnidiStreamCount = *value;
			quic.IsSet.PeerUnidiStreamCount = TRUE;
		}

		return quic;
	}

	std::optional<uint64_t> QuicSettingsParser::getOptionalUint(
		const structures::ExternalConnectionSettings &settings,
		std::string_view key
	) {
		const auto it = settings.protocolSettings.find(std::string(key));
		if (it == settings.protocolSettings.end()) {
			return std::nullopt;
		}

		const auto &raw = it->second;

		if (!nlohmann::json::accept(raw)) {
			settings::Logger::logWarning(
				"[quic] QUIC setting '{}' is not valid JSON", key
			);
			return std::nullopt;
		}

		auto j = nlohmann::json::parse(raw);

		if (!j.is_number_unsigned()) {
			settings::Logger::logWarning(
				"[quic] QUIC setting '{}' must be an unsigned integer", key
			);
			return std::nullopt;
		}

		return j.get<uint64_t>();
	}
}
