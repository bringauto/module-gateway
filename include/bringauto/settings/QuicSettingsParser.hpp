#pragma once

#include <bringauto/structures/ExternalConnectionSettings.hpp>

#include <msquic.h>

#include <optional>


namespace bringauto::settings {
	class QuicSettingsParser {
	public:
		static QUIC_SETTINGS parse(const structures::ExternalConnectionSettings &settings);

	private:
		static std::optional<uint64_t> getOptionalUint(
			const structures::ExternalConnectionSettings &settings,
			std::string_view key
		);
	};
}
