#pragma once

namespace bringauto::external_client::connection {

enum ConnectionState {
	/**
	 * NOT_CONNECTED - Client is not connected to the External server
	 */
	NOT_CONNECTED,
	/**
	 * CONNECTING - Client is in the connect sequence to server
	 */
	CONNECTING,
	/**
	 * CONNECTED - Client is connected to the External server
	 */
	 CONNECTED
};
}