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
