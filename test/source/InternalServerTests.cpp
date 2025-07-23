#include <InternalServerTests.hpp>


InternalProtocol::Device
createDevice(int module, unsigned int type, const std::string &role, const std::string &name,
		unsigned int priority) {
	InternalProtocol::Device device {};
	device.set_module(static_cast<InternalProtocol::Device::Module>(module));
	device.set_devicetype(type);
	device.set_devicerole(role);
	device.set_devicename(name);
	device.set_priority(priority);
	return device;
}

///TESTS FOR MULTIPLE CONNECTIONS
/**
 * @brief Tests Connection of one client, and communication between client-server-handler
 */
TEST_F(InternalServerTests, OneClient) {
	std::vector<InternalProtocol::Device> devices {
		createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			defaultPriority
		)
	};
	std::vector<std::string> data { defaultData };
	testing_utils::TestHandler testedData(devices, data);
	testedData.runTestsParallelConnections();
}

/**
 * @brief Tests Connection of 2 clients, and communication between clients-server-handler
 */
TEST_F(InternalServerTests, TwoClients) {
	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 1; i <= 2; ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData + std::to_string(i));
	}
	testing_utils::TestHandler testedData(devices, data);
	testedData.runTestsParallelConnections();
}

/**
 * @brief Tests Connection of 5 clients, and communication between clients-server-handler
 */
TEST_F(InternalServerTests, FiveClients) {
	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 1; i <= 5; ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData + std::to_string(i));
	}
	testing_utils::TestHandler testedData(devices, data);
	testedData.runTestsParallelConnections();
}

/**
 * @brief Tests Connection of 50 clients, and communication between clients-server-handler
 */
TEST_F(InternalServerTests, FiftyClients) {
	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 1; i <= 50; ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData + std::to_string(i));
	}
	testing_utils::TestHandler testedData(devices, data);
	testedData.runTestsParallelConnections();
}

///TESTS FOR RESPONSES TO DIFFERENT PRIORITES
/**
 * @brief tests if server responds to each client with correct response and running communication is not broken
 */
TEST_F(InternalServerTests, SameRolePriority000) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 0, 0, 0 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response and running communication is not broken
 */
TEST_F(InternalServerTests, SameRolePriority001) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED,
		InternalProtocol::
		DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 0, 0, 1 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response, lower priority connection is disconnected correctly and no communication is broken
 */
TEST_F(InternalServerTests, SameRolePriority110) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};
	std::vector<size_t> priorities { 1, 1, 0 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response and running communication is not broken
 */
TEST_F(InternalServerTests, SameRolePriority121) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 1, 2, 1 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response, lower priority connection is disconnected correctly and no communication is broken
 */
TEST_F(InternalServerTests, SameRolePriority101) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 1, 0, 1 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response and running communication is not broken
 */
TEST_F(InternalServerTests, SameRolePriority122) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED,
		InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 1, 2, 2 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response, lower priority connection is disconnected correctly and no communication is broken
 */
TEST_F(InternalServerTests, SameRolePriority120) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_HIGHER_PRIORITY_ALREADY_CONNECTED,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};
	std::vector<size_t> priorities { 1, 2, 0 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response, lower priority connection is disconnected correctly and no communication is broken
 */
TEST_F(InternalServerTests, SameRolePriority210) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};
	std::vector<size_t> priorities { 2, 1, 0 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if server responds to each client with correct response, lower priority connection is disconnected correctly and no communication is broken
 */
TEST_F(InternalServerTests, SameRolePriority211) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_ALREADY_CONNECTED
	};
	std::vector<size_t> priorities { 2, 1, 1 };

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(auto &priority: priorities) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole,
			defaultName,
			priority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsSerialConnections();
}

/**
 * @brief tests if connection is rejected when we send less than 4 bytes of a message and other communication is not broken
 */
TEST_F(InternalServerTests, RejectMessageSmallerThan4Bytes) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	auto connectStr = testedData.getConnect(0).SerializeAsString();
	testedData.runTestsWithWrongMessage(1, connectStr.size(), "", false, true);
}

/**
 * @brief tests if connection is rejected when we send 4 bytes of the message (as the header) defining size of the rest of message as 0 and other communication is not broken
 */
TEST_F(InternalServerTests, RejectMesseageComposedOfOnlyHeaderWithNumber0) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsWithWrongMessage(1, 0, "", true);
}

/**
 * this test will only make sense when processBufferData returns false if bytesTransferred is 0 and header value is not 0 (not intended as of v1.3.5)
 */
TEST_F(InternalServerTests, RejectMessageComposedOfOnlyHeader) {
	GTEST_SKIP();
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	auto connectStr = testedData.getConnect(0).SerializeAsString();
	testedData.runTestsWithWrongMessage(0, connectStr.size(), "", true);
}

/**
 * @brief tests if connection is rejected when we send message that is not matching InternalProtocol::InternalClient message and other communication is not broken
 */
TEST_F(InternalServerTests, RejectMessageWithGarbageDataMatchingHeaderSize) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	auto connectStr = testedData.getConnect(0).SerializeAsString();
	testedData.runTestsWithWrongMessage(1, connectStr.size() + 5, connectStr + "12345", true);
}

/**
 * this test will only make sense if processBufferData returns false if bytesTransferred is less than header size and header value is not 0 (not intended as of v1.3.5)
 */
TEST_F(InternalServerTests, RejectMessageWithLessDataThanHeaderSays) {
	GTEST_SKIP();
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	auto connectStr = testedData.getConnect(0).SerializeAsString();
	testedData.runTestsWithWrongMessage(1, connectStr.size(), "12345", false);
}

/**
 * @brief tests for disconnection when device is not connected and first message is status
 */
TEST_F(InternalServerTests, RejectMessageWhereStatusIsSentBeforeConnection) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.changeConnectionIntoStatus(1);
	auto connectStr = testedData.getConnect(1).SerializeAsString();
	testedData.runTestsWithWrongMessage(1, connectStr.size(), connectStr, true);
}

/**
 * @brief tests for disconnection when client receives connect message on connection that is already connected
 */
TEST_F(InternalServerTests, RejectMessageWhereConnectionIsSentAfterAlreadyBeingConnected) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.changeStatusIntoConnection(0);
	auto connectStr = testedData.getConnect(0).SerializeAsString();
	testedData.runTestsWithWrongMessage(0, connectStr.size(), connectStr, false);
}

/**
 * @brief tests if server correctly disconnects if it does not receive response to connect from module hanlder in itme
 */
TEST_F(InternalServerTests, TestForBehaviorWhereModuleHandlerDoesntRespondToConnect) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsWithModuleHandlerTimeout(true, 2);
}

/**
 * @brief tests if server correctly disconnects if it does not receive command to status from module hanlder in time
 */
TEST_F(InternalServerTests, TestForBehaviorWhereModuleHandlerDoesntRespondToStatus) {
	std::vector<InternalProtocol::DeviceConnectResponse_ResponseType> responseType {
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK,
		InternalProtocol::DeviceConnectResponse_ResponseType_OK
	};

	std::vector<InternalProtocol::Device> devices {};
	std::vector<std::string> data {};
	for(size_t i = 0; i < responseType.size(); ++i) {
		devices.emplace_back(createDevice(
			defaultModule,
			defaultType,
			defaultRole + std::to_string(i),
			defaultName + std::to_string(i),
			defaultPriority
		));
		data.push_back(defaultData);
	}
	testing_utils::TestHandler testedData(devices, responseType, data);
	testedData.runTestsWithModuleHandlerTimeout(false, 2);
}
