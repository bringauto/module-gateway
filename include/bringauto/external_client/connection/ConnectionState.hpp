#pragma once

namespace bringauto::external_client::connection {

enum class ConnectionState {
	/**
	 * NOT_INITIALIZED - Client is not Initialized yet
	 */
	NOT_INITIALIZED,
	/**
	 * NOT_CONNECTED - Client failed to connect to the External server
	 */
	NOT_CONNECTED,
	/**
	 * CONNECTING - Client is in the connect sequence to server
	 */
	CONNECTING,
	/**
	 * CONNECTED - Client is connected to the External server
	 */
	CONNECTED,
	/**
	 * CLOSING - Client closing connection to the External server
	 */
	CLOSING
};
}
