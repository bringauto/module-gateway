#include <testing_utils/TestHandler.hpp>
#include <testing_utils/ProtobufUtils.hpp>

#include <thread>



namespace testing_utils {

namespace common_utils = bringauto::common_utils;
namespace internal_server = bringauto::internal_server;
namespace settings = bringauto::settings;
namespace structures = bringauto::structures;



TestHandler::TestHandler(const std::vector <InternalProtocol::Device> &devices, const std::vector <std::string> &data) {
	settings = std::make_shared<settings::Settings>();
	settings->port = port;

	toInternalQueue = std::make_shared < structures::AtomicQueue < structures::ModuleHandlerMessage >> ();
	fromInternalQueue = std::make_shared < structures::AtomicQueue < structures::InternalClientMessage >> ();
	for(size_t i = 0; i < devices.size(); ++i) {
		connects.push_back(ProtobufUtils::CreateClientMessage(devices[i]));
		statuses.push_back(ProtobufUtils::CreateClientMessage(devices[i], data[i]));
		commands.push_back(ProtobufUtils::CreateServerMessage(devices[i], data[i]));
		responses.push_back(ProtobufUtils::CreateServerMessage(
			devices[i],
			InternalProtocol::DeviceConnectResponse_ResponseType_OK
		));

		contexts.push_back(std::make_shared<structures::GlobalContext>());
		contexts[i]->settings = std::make_shared<settings::Settings>();
		contexts[i]->settings->port = (port);
		clients.emplace_back(contexts[i]);
		expectedMessageNumber += numberOfMessages;
		for(size_t y = 0; y < i; ++y) {
			auto a = std::make_shared<structures::DeviceIdentification>(devices[y]);
			auto b = std::make_shared<structures::DeviceIdentification>(devices[i]);
			if(a->isSame(b)) {
				expectedMessageNumber -= (numberOfMessages - 1);
				break;
			}
		}
	}
}

TestHandler::TestHandler(
		const std::vector <InternalProtocol::Device> &devices,
		const std::vector <InternalProtocol::DeviceConnectResponse_ResponseType> &responseTypes,
		const std::vector <std::string> &data) {
	settings = std::make_shared<settings::Settings>();
	settings->port = port;

	toInternalQueue = std::make_shared < structures::AtomicQueue < structures::ModuleHandlerMessage >> ();
	fromInternalQueue = std::make_shared < structures::AtomicQueue < structures::InternalClientMessage >> ();
	for(size_t i = 0; i < devices.size(); ++i) {
		connects.push_back(ProtobufUtils::CreateClientMessage(devices[i]));
		statuses.push_back(ProtobufUtils::CreateClientMessage(devices[i], data[i]));
		commands.push_back(ProtobufUtils::CreateServerMessage(devices[i], data[i]));
		responses.push_back(ProtobufUtils::CreateServerMessage(devices[i], responseTypes[i]));

		contexts.push_back(std::make_shared<structures::GlobalContext>());
		contexts[i]->settings = std::make_shared<settings::Settings>();
		contexts[i]->settings->port = (port);
		clients.emplace_back(contexts[i]);
		if(responseTypes[i] == InternalProtocol::DeviceConnectResponse_ResponseType_OK) {
			expectedMessageNumber += numberOfMessages;
			for(size_t y = 0; y < i; ++y) {
				auto a = std::make_shared<structures::DeviceIdentification>(devices[y]);
				auto b = std::make_shared<structures::DeviceIdentification>(devices[i]);
				if(a->isSame(b)) {
					expectedMessageNumber -= (numberOfMessages - 1);
					break;
				}
			}
		}
	}
}

InternalProtocol::InternalClient TestHandler::getConnect(size_t index) {
	return connects[index];
}

void TestHandler::changeConnectionIntoStatus(size_t index) {
	connects[index] = statuses[index];
}

void TestHandler::changeStatusIntoConnection(size_t index) {
	statuses[index] = connects[index];
}

void TestHandler::ParallelRun(size_t index) {
	size_t messagesSent { 0 };
	clients[index].connectSocket();
	clients[index].sendMessage(connects[index]);
	++messagesSent;
	InternalProtocol::InternalServer receivedMessage {};
	clients[index].receiveMessage(receivedMessage);
	ASSERT_EQ(receivedMessage.SerializeAsString(), responses[index].SerializeAsString());
	if(receivedMessage.deviceconnectresponse().responsetype() !=
	   InternalProtocol::DeviceConnectResponse_ResponseType_OK) {
		return;
	}
	while(messagesSent < numberOfMessages) {
		clients[index].sendMessage(statuses[index]);
		++messagesSent;
		clients[index].receiveMessage(receivedMessage);
		ASSERT_EQ(receivedMessage.SerializeAsString(), commands[index].SerializeAsString());
	}
	clients[index].disconnectSocket();
}

void TestHandler::runTestsParallelConnections() {
	auto context = std::make_shared<structures::GlobalContext>(settings);

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	internal_server::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };
	testing_utils::ModuleHandlerForTesting moduleHandler(context, fromInternalQueue, toInternalQueue,
		expectedMessageNumber);

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.start(); });
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.run();

	std::vector <std::jthread> clientThreads {};
	for(size_t i = 0; i < responses.size(); ++i) {
		clientThreads.emplace_back([this, i]() { (ParallelRun(i)); });
	}
	for(size_t i = 0; i < responses.size(); ++i) {
		clientThreads[i].join();
	}

	if(!context->ioContext.stopped()) {
		context->ioContext.stop();
	}
	internalServer.destroy();
}

void TestHandler::runConnects() {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size(); ++i) {
		clients[i].connectSocket();
		clients[i].sendMessage(connects[i]);
		clients[i].receiveMessage(receivedMessage);
		ASSERT_EQ(receivedMessage.SerializeAsString(), responses[i].SerializeAsString());
		if(receivedMessage.deviceconnectresponse().responsetype() !=
		   InternalProtocol::DeviceConnectResponse_ResponseType_OK) {
			clients[i].disconnectSocket();
		} else {
			for(size_t y = 0; y < i; ++y) {
				auto a = std::make_shared<structures::DeviceIdentification>(
					connects[y].deviceconnect().device());
				auto b = std::make_shared<structures::DeviceIdentification>(
					connects[i].deviceconnect().device());
				if(a->isSame(b)) {
					clients[y].disconnectSocket();
				}
			}
		}
	}
}

void TestHandler::runStatuses() {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size()*(numberOfMessages - 1); ++i) {
		if(clients[i%clients.size()].isOpen()) {
			clients[i%clients.size()].sendMessage(statuses[i%clients.size()]);
			clients[i%clients.size()].receiveMessage(receivedMessage);
			ASSERT_EQ(receivedMessage.SerializeAsString(), commands[i%clients.size()].SerializeAsString());
		}
	}
}

void TestHandler::disconnectAll() {
	for(size_t i = 0; i < clients.size(); ++i) {
		clients[i].disconnectSocket();
	}
}

void TestHandler::serialRun() {
	runConnects();
	runStatuses();
	disconnectAll();
}

void TestHandler::runTestsSerialConnections() {
	auto context = std::make_shared<structures::GlobalContext>(settings);

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	internal_server::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };

	testing_utils::ModuleHandlerForTesting moduleHandler(context, fromInternalQueue, toInternalQueue,
		expectedMessageNumber);

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.start(); });
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.run();

	serialRun();

	if(!context->ioContext.stopped()) {
		context->ioContext.stop();
	}
	internalServer.destroy();
}

void TestHandler::runConnects(size_t index, size_t header, std::string data, bool recastHeader) {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size(); ++i) {
		if(index == i) {
			clients[i].connectSocket();
			clients[i].sendMessage(header, data, recastHeader);
			clients[i].insteadOfMessageExpectError();
			clients[i].disconnectSocket();
		} else {
			clients[i].connectSocket();
			clients[i].sendMessage(connects[i]);
			clients[i].receiveMessage(receivedMessage);
			ASSERT_EQ(receivedMessage.SerializeAsString(), responses[i].SerializeAsString());
			if(receivedMessage.deviceconnectresponse().responsetype() !=
			   InternalProtocol::DeviceConnectResponse_ResponseType_OK) {
				clients[i].disconnectSocket();
			} else {
				for(size_t y = 0; y < i; ++y) {
					auto a = std::make_shared<structures::DeviceIdentification>(
						connects[y].deviceconnect().device());
					auto b = std::make_shared<structures::DeviceIdentification>(
						connects[i].deviceconnect().device());
					if(a->isSame(b)) {
						clients[y].disconnectSocket();
					}
				}
			}
		}
	}
}

void TestHandler::runStatuses(size_t index, size_t header, std::string data, bool recastHeader) {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size()*(numberOfMessages - 1); ++i) {
		if(index == i) {
			clients[i].sendMessage(header, data, recastHeader);
			clients[i].insteadOfMessageExpectError();
			clients[i].disconnectSocket();
		} else if(clients[i%clients.size()].isOpen()) {
			clients[i%clients.size()].sendMessage(statuses[i%clients.size()]);
			clients[i%clients.size()].receiveMessage(receivedMessage);
			ASSERT_EQ(receivedMessage.SerializeAsString(), commands[i%clients.size()].SerializeAsString());
		}
	}
}

void TestHandler::serialRunWithExpectedError(size_t index, size_t header, std::string data, bool onConnect,
		bool recastHeader) {
	if(onConnect) {
		runConnects(index, header, data, recastHeader);
		runStatuses();
	} else {
		runConnects();
		runStatuses(index, header, data, recastHeader);
	}
	disconnectAll();
}

void TestHandler::runTestsWithWrongMessage(size_t index, uint32_t header, std::string data, bool onConnect,
		bool recastHeader) {
	auto context = std::make_shared<structures::GlobalContext>(settings);

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	internal_server::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };

	if(onConnect) {
		expectedMessageNumber -= numberOfMessages;
	} else {
		expectedMessageNumber -= (numberOfMessages - 1);
	}
	testing_utils::ModuleHandlerForTesting moduleHandler(context, fromInternalQueue, toInternalQueue,
		expectedMessageNumber);

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.start(); });
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.run();

	serialRunWithExpectedError(index, header, data, onConnect, recastHeader);

	if(!context->ioContext.stopped()) {
		context->ioContext.stop();
	}
	internalServer.destroy();
}

void TestHandler::runConnects(size_t numberOfErrors) {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size(); ++i) {
		if(i < numberOfErrors) {
			clients[i].connectSocket();
			clients[i].sendMessage(connects[i]);
			clients[i].insteadOfMessageExpectError();
			clients[i].disconnectSocket();
		} else {
			clients[i].connectSocket();
			clients[i].sendMessage(connects[i]);
			clients[i].receiveMessage(receivedMessage);
			ASSERT_EQ(receivedMessage.SerializeAsString(), responses[i].SerializeAsString());
			if(receivedMessage.deviceconnectresponse().responsetype() !=
			   InternalProtocol::DeviceConnectResponse_ResponseType_OK) {
				clients[i].disconnectSocket();
			} else {
				for(size_t y = 0; y < i; ++y) {
					auto a = std::make_shared<structures::DeviceIdentification>(
						connects[y].deviceconnect().device());
					auto b = std::make_shared<structures::DeviceIdentification>(
						connects[i].deviceconnect().device());
					if(a->isSame(b)) {
						clients[y].disconnectSocket();
					}
				}
			}
		}
	}
}

void TestHandler::runStatuses(size_t numberOfErrors) {
	InternalProtocol::InternalServer receivedMessage {};
	for(size_t i = 0; i < clients.size()*(numberOfMessages - 1); ++i) {
		if(i < numberOfErrors) {
			clients[i].sendMessage(statuses[i]);
			clients[i].insteadOfMessageExpectError();
			clients[i].disconnectSocket();
		} else if(clients[i%clients.size()].isOpen()) {
			clients[i%clients.size()].sendMessage(statuses[i%clients.size()]);
			clients[i%clients.size()].receiveMessage(receivedMessage);
			ASSERT_EQ(receivedMessage.SerializeAsString(), commands[i%clients.size()].SerializeAsString());
		}
	}
}

void TestHandler::serialRunWithExpectedError(bool onConnect, size_t numberOfErrors) {
	if(onConnect) {
		runConnects(numberOfErrors);
		runStatuses();
	} else {
		runConnects();
		runStatuses(numberOfErrors);
	}
	disconnectAll();
}

void TestHandler::runTestsWithModuleHandlerTimeout(bool onConnect, size_t timeoutNumber) {
	auto context = std::make_shared<structures::GlobalContext>(settings);

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	internal_server::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };

	if(onConnect) {
		expectedMessageNumber -= timeoutNumber*numberOfMessages;
	} else {
		expectedMessageNumber -= timeoutNumber*(numberOfMessages - 1);
	}
	testing_utils::ModuleHandlerForTesting moduleHandler(context, fromInternalQueue, toInternalQueue,
		expectedMessageNumber);

	std::jthread moduleHandlerThread([&moduleHandler, onConnect, timeoutNumber]() {
		moduleHandler.startWithTimeout(onConnect, timeoutNumber);
	});
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.run();

	serialRunWithExpectedError(onConnect, timeoutNumber);


	if(!context->ioContext.stopped()) {
		context->ioContext.stop();
	}
	internalServer.destroy();
}


}
