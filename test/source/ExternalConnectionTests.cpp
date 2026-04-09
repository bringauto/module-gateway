#include "ExternalConnectionTests.hpp"


/**
 * @brief Test the connection sequence and expect a successful connection
 */
TEST_F(ExternalConnectionTests, SuccessfulConnectionSequence) {
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::NOT_INITIALIZED);
	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), 0);
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::CONNECTED);
}


/**
 * @brief Expect a failed connection sequence when unable to connect
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnInitialization) {
	communicationChannel_->setFailOnInitConnection(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when unable to send a message
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnConnectSend) {
	communicationChannel_->setFailOnSend(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received connect response is a nullptr
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnConnectReceiveNullptr) {
	communicationChannel_->setConnectMsgNullptr(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received message is not a connect response
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnConnectReceiveNoType) {
	communicationChannel_->setConnectMsgNoType(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received connect response has a different session id
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnConnectReceiveBadSessionId) {
	communicationChannel_->setConnectMsgBadSessionId(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received connect response is already logged in
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnConnectReceiveAlreadyLoggedIn) {
	communicationChannel_->setConnectMsgAlreadyLoggedIn(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received status response is a nullptr
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnStatusReceiveNullptr) {
	communicationChannel_->setStatusMsgNullptr(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received message is not a status response
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnStatusReceiveNoType) {
	communicationChannel_->setStatusMsgNoType(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received status response is not ok
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnStatusReceiveNotOk) {
	communicationChannel_->setStatusMsgNotOk(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received status response has a different session id
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnStatusReceiveBadSessionId) {
	communicationChannel_->setStatusMsgBadSessionId(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received command is a nullptr
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnCommandReceiveNullptr) {
	communicationChannel_->setCommandMsgNullptr(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received message is not a command
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnCommandReceiveNoType) {
	communicationChannel_->setCommandMsgNoType(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received command has a different session id
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnCommandReceiveBadSessionId) {
	communicationChannel_->setCommandMsgBadSessionId(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Expect a failed connection sequence when the received command is for a module that does not exist
 */
TEST_F(ExternalConnectionTests, FailedConnectionSequenceOnCommandModuleNotExists) {
	communicationChannel_->setCommandMsgModuleNotExists(true);
	expectFailedConnectionSequence();
}


/**
 * @brief Trigger a server-side disconnect after a successful handshake.
 * Verify that the receiving loop detects the disconnect and pushes an item
 * onto the reconnect queue.
 */
TEST_F(ExternalConnectionTests, DisconnectDetectionPushesToReconnectQueue) {
	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), 0);
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::CONNECTED);

	communicationChannel_->setConnected(false);

	// The receiving loop calls receiveMessage() -> nullptr, then isConnected() -> false,
	// and pushes to reconnectQueue_ via pushAndNotify. Wait up to 5 seconds.
	const bool queueStillEmpty = reconnectQueue_->waitForValueWithTimeout(std::chrono::seconds(5));
	ASSERT_FALSE(queueStillEmpty);
}


/**
 * @brief When receiveMessage() returns nullptr but isConnected() is still true (timeout path),
 * the receiving loop must NOT push anything onto the reconnect queue.
 */
TEST_F(ExternalConnectionTests, ReceiveTimeoutWithConnectedDoesNotTriggerReconnect) {
	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), 0);
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::CONNECTED);

	// After the handshake, nextMessageType_ is CONNECT_RESPONSE.
	// setConnectMsgNullptr(true) makes receiveMessage() return nullptr while isConnected() stays true.
	communicationChannel_->setConnectMsgNullptr(true);

	// Wait a short time and verify nothing was pushed to the reconnect queue.
	const bool queueStillEmpty = reconnectQueue_->waitForValueWithTimeout(std::chrono::seconds(2));
	ASSERT_TRUE(queueStillEmpty);
}


/**
 * @brief Regression test for the stopReceiving reset race fixed in deinitializeConnection.
 * Simulate: successful connect -> disconnect detection -> deinitializeConnection -> initializeConnection.
 * Verify the new listening thread is actually running by triggering a second disconnect detection.
 */
TEST_F(ExternalConnectionTests, ReinitializationAfterDisconnectTriggeredExit) {
	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), 0);
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::CONNECTED);

	// First disconnect: loop sees nullptr + isConnected()==false and exits via return.
	communicationChannel_->setConnected(false);
	const bool firstQueueEmpty = reconnectQueue_->waitForValueWithTimeout(std::chrono::seconds(5));
	ASSERT_FALSE(firstQueueEmpty);

	// Drain the reconnect queue item from the first disconnect.
	reconnectQueue_->pop();

	// Deinitialize. The fix in deinitializeConnection ensures stopReceiving is cleared after join,
	// eliminating the race where the new thread could see stopReceiving==true on startup.
	externalConnection_->deinitializeConnection(false);

	// Restore mock to a working state for the second handshake.
	communicationChannel_->setConnected(true);

	// Re-populate the error aggregator: initializeConnection's statusMessageHandle requires
	// a last status for each device. The first run cleared it via clear_error_aggregator().
	externalConnection_->fillErrorAggregator(bringauto::common_utils::ProtobufUtils::createDeviceStatus(
		connectedDevices_[0], create_buffer("status")));

	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), 0);
	ASSERT_EQ(externalConnection_->getState(), bringauto::external_client::connection::ConnectionState::CONNECTED);

	// Trigger a second disconnect. If the new listening thread is running, it will detect
	// the disconnect and push onto reconnectQueue_. If stopReceiving were stuck true,
	// the thread would have exited without ever polling receiveMessage() and nothing would
	// be pushed.
	communicationChannel_->setConnected(false);
	const bool secondQueueEmpty = reconnectQueue_->waitForValueWithTimeout(std::chrono::seconds(5));
	ASSERT_FALSE(secondQueueEmpty);
}


/**
 * @brief Test the changing of the session id.
 * The session id should change every time the connection is initialized, and its length should be 8.
 */
TEST_F(ExternalConnectionTests, SessionIdChangingCorrectly) {
	communicationChannel_->setConnectMsgNullptr(true);
	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), -1);
	std::string sessionId = communicationChannel_->getSessionId();
	ASSERT_EQ(sessionId.length(), EXPECTED_SESSION_ID_LENGTH);

	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), -1);
	std::string newSessionId = communicationChannel_->getSessionId();
	ASSERT_EQ(newSessionId.length(), EXPECTED_SESSION_ID_LENGTH);
	ASSERT_NE(sessionId, newSessionId);
	sessionId = newSessionId;

	ASSERT_EQ(externalConnection_->initializeConnection(connectedDevices_), -1);
	newSessionId = communicationChannel_->getSessionId();
	ASSERT_EQ(newSessionId.length(), EXPECTED_SESSION_ID_LENGTH);
	ASSERT_NE(sessionId, newSessionId);
}
