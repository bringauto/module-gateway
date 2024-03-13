#pragma once

#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/structures/DeviceIdentification.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <testing_utils/InternalClientForTesting.hpp>
#include <testing_utils/ModuleHandlerForTesting.hpp>

#include <memory>
#include <vector>



namespace testing_utils {

const size_t numberOfMessages { 100 };
const unsigned short port { 8888 };

class TestHandler {
	std::shared_ptr<bringauto::settings::Settings> settings;

	std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::ModuleHandlerMessage>> toInternalQueue;
	std::shared_ptr<bringauto::structures::AtomicQueue<bringauto::structures::InternalClientMessage>> fromInternalQueue;

	std::vector<InternalProtocol::InternalClient> connects;
	std::vector<InternalProtocol::InternalClient> statuses;
	std::vector<InternalProtocol::InternalServer> commands;
	std::vector<InternalProtocol::InternalServer> responses;
	std::vector<std::shared_ptr<bringauto::structures::GlobalContext>> contexts;

	std::vector<testing_utils::ClientForTesting> clients;
	size_t expectedMessageNumber { 0 };

public:

	TestHandler(const std::vector<InternalProtocol::Device> &devices, const std::vector<std::string> &data);

	TestHandler(
		const std::vector<InternalProtocol::Device> &devices,
		const std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> &responseTypes,
		const std::vector<std::string> &data
	);

	InternalProtocol::InternalClient getConnect(size_t index);

	void changeConnectionIntoStatus(size_t index);

	void changeStatusIntoConnection(size_t index);

	void ParallelRun(size_t index);

	void runTestsParallelConnections();

	void runConnects();
	void runStatuses();
	void disconnectAll();
	void serialRun();
	void runTestsSerialConnections();

	void runConnects(size_t index, size_t header, std::string data,  bool recastHeader);
	void runStatuses(size_t index, size_t header, std::string data,  bool recastHeader);
	void serialRunWithExpectedError(size_t index, size_t header, std::string data, bool onConnect, bool recastHeader);
	void runTestsWithWrongMessage(size_t index, uint32_t header, std::string data, bool onConnect,
		bool recastHeader = false);

	void runConnects(size_t numberOfErrors);
	void runStatuses(size_t numberOfErrors);
	void serialRunWithExpectedError(bool onConnect, size_t numberOfErrors);

	void runTestsWithModuleHandlerTimeout(bool onConnect, size_t timeoutNumber);
};

}