#pragma once

#include <bringauto/settings/Constants.hpp>
#include <array>
#include <vector>



namespace bringauto::structures {


/**
 * @brief Context for each accepted and active connection. Is used for processing received data.
 */
struct ConnectionContext {
	/**
	 * @brief buffer for receive handler
	 */
	std::array<uint8_t, bringauto::settings::buffer_length> buffer;
	/**
	 * @brief complete size of one message
	 */
	std::size_t completeMessageSize { 0 };
	/**
	 * @brief message data
	 */
	std::vector<uint8_t> completeMessage {};
};

}