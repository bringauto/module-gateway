#include <testing_utils/InternalClientForTesting.hpp>

#include <vector>
#include <thread>



namespace testing_utils {

void ClientForTesting::connectSocket() {
	boost::system::error_code er;
	boost::asio::ip::tcp::resolver resolver(context->ioContext);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), context->settings->port);
	socket = std::make_shared<boost::asio::ip::tcp::socket>(context->ioContext);
	socket->connect(endpoint, er);
	ASSERT_FALSE(er);
}

void ClientForTesting::disconnectSocket() {
	if(socket->is_open()) {
		socket->shutdown(boost::asio::socket_base::shutdown_both);
		socket->close();
	}
}

bool ClientForTesting::isOpen() {
	return socket->is_open();
}

void ClientForTesting::sendMessage(const InternalProtocol::InternalClient &message) {
	std::string data = message.SerializeAsString();
	uint32_t header = data.size();
	auto headerWSize = socket->write_some(boost::asio::buffer(&header, sizeof(uint32_t)));
	ASSERT_EQ(headerWSize, sizeof(uint32_t));
	auto dataWSize = socket->write_some(boost::asio::buffer(data));
	ASSERT_EQ(dataWSize, header);
}

void ClientForTesting::receiveMessage(InternalProtocol::InternalServer &message) {
	boost::system::error_code er;
	bool readFinished = false;
	auto timeStart = std::chrono::steady_clock::now();
	std::thread timeoutCountThread([this, timeStart, &readFinished]() {
		while(std::chrono::duration<double>(std::chrono::steady_clock::now() - timeStart) <
			  timeoutLengthGreaterThenDefinedInFleetProtocol) {
			if(readFinished) {
				return;
			}
		}
		disconnectSocket();
		ASSERT_TRUE(false);
	});
	std::array<uint8_t, bufferLength> buffer;
	size_t bytesTransferred = socket->read_some(boost::asio::buffer(buffer), er);
	if(er) {
		readFinished = true;
		timeoutCountThread.join();
		ASSERT_FALSE(er);
	}
	if(bytesTransferred < headerSize) {
		readFinished = true;
		timeoutCountThread.join();
		ASSERT_GE(bytesTransferred, headerSize);
	}
	uint32_t size { 0 };
	std::copy(buffer.begin(), buffer.begin() + headerSize, (uint8_t *)&size);
	if(bytesTransferred < headerSize + size) {
		while(socket->available() < size) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
		bytesTransferred = socket->read_some(boost::asio::buffer(buffer), er);
		if(er) {
			readFinished = true;
			timeoutCountThread.join();
			ASSERT_FALSE(er);
		}
	}
	readFinished = true;
	timeoutCountThread.join();

	std::vector<uint8_t> vector;
	vector.reserve(size);
	if(bytesTransferred == size) {
		std::copy(buffer.begin(), buffer.begin() + size, std::back_inserter(vector));
	} else {
		std::copy(buffer.begin() + headerSize, buffer.begin() + headerSize + size, std::back_inserter(vector));
	}
	ASSERT_TRUE(message.ParseFromArray(vector.data(), size));
}

void ClientForTesting::sendMessage(uint32_t header, std::string data, bool recastHeader) {
	if(recastHeader) {
		socket->write_some(boost::asio::buffer(&header, sizeof(uint16_t)));
		return;
	}
	auto headerWSize = socket->write_some(boost::asio::buffer(&header, sizeof(uint32_t)));
	ASSERT_EQ(headerWSize, sizeof(uint32_t));
	auto dataWSize = socket->write_some(boost::asio::buffer(data));
	ASSERT_EQ(dataWSize, data.size());
}

void ClientForTesting::insteadOfMessageExpectError() {
	boost::system::error_code er;
	bool readFinished = false;
	auto timeStart = std::chrono::steady_clock::now();
	std::thread timeoutCountThread([this, timeStart, &readFinished]() {
		while(std::chrono::duration<double>(std::chrono::steady_clock::now() - timeStart) <
			  timeoutLengthGreaterThenDefinedInFleetProtocol) {
			if(readFinished) {
				return;
			}
		}
		disconnectSocket();
		ASSERT_TRUE(false);
	});
	std::array<uint8_t, bufferLength> buffer;
	socket->read_some(boost::asio::buffer(buffer), er);
	readFinished = true;
	timeoutCountThread.join();
	ASSERT_TRUE(er);
}

void ClientForTesting::insteadOfMessageExpectTimeoutThenError() {
	boost::system::error_code er;
	bool readFinished = false;
	auto timeStart = std::chrono::steady_clock::now();
	std::thread timeoutCountThread([this, timeStart, &readFinished]() {
		while(std::chrono::duration<double>(std::chrono::steady_clock::now() - timeStart) <
			  timeoutLengthDefinedInFleetProtocol) {
			if(readFinished) {
				disconnectSocket();
				ASSERT_TRUE(false);
			}
		}
	});
	std::array<uint8_t, bufferLength> buffer;
	socket->read_some(boost::asio::buffer(buffer), er);
	readFinished = true;
	timeoutCountThread.join();
	ASSERT_TRUE(er);

}
}