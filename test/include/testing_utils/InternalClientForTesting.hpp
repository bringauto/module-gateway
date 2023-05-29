#pragma once

#include <InternalProtocol.pb.h>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/AtomicQueue.hpp>

#include <gtest/gtest.h>
#include <memory>
#include <string>



namespace testing_utils {

const std::chrono::seconds timeoutLengthDefinedInFleetProtocol { 30 };
const std::chrono::seconds timeoutLengthGreaterThenDefinedInFleetProtocol { 40 };
const size_t bufferLength { 1024 };
const size_t headerSize { 4 };

class ClientForTesting {
	const std::shared_ptr<bringauto::structures::GlobalContext> context;
	std::shared_ptr<boost::asio::ip::tcp::socket> socket;
public:

	ClientForTesting(const std::shared_ptr<bringauto::structures::GlobalContext> &context_)
			: context(context_) {}

	void connectSocket();

	void disconnectSocket();

	bool isOpen();

	void sendMessage(const InternalProtocol::InternalClient &message);

	void sendMessage(uint32_t header, std::string data, bool recastHeader);

	void receiveMessage(InternalProtocol::InternalServer &message);

	void insteadOfMessageExpectError();

	void insteadOfMessageExpectTimeoutThenError();
};

}