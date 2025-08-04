#pragma once

#include <Aeron.h>
#include <FragmentAssembler.h>
#include <concurrent/YieldingIdleStrategy.h>



namespace bringauto::aeron_communication {

/**
 * This class is responsible for managing Aeron communication.
 * It is used for calling module functions on a separate module binary using Aeron.
 */
class AeronClient {

public:
	enum ModuleFunctions {
		ALLOCATE = 0,
		AGGREGATE_ERROR = 1,
		AGGREGATE_STATUS = 2,
		COMMAND_DATA_VALID = 3,
		DEALLOCATE = 4,
		GENERATE_COMMAND = 5,
		GENERATE_FIRST_COMMAND = 6,
		SEND_STATUS_CONDITION = 7,
		STATUS_DATA_VALID = 8
	};

	/**
	 * @brief Constructs an AeronClient instance.
	 * Initializes the Aeron context and sets up handlers for publications and subscriptions.
	 */
	AeronClient();

	~AeronClient() = default;

	/**
	 * @brief Adds a module to the Aeron client with the given module ID.
	 * Creates a publication and subscription for the module.
	 * Only supports one module for now.
	 * @param moduleId The ID of the module to be added.
	 */
	void addModule(uint16_t moduleId);

	/**
	 * @brief Calls a module function with the specified message.
	 * Constructs a full message by combining the function code and the provided message,
	 * then sends it via Aeron publication.
	 * @param function The module function to call.
	 * @param message The message to send to the module.
	 */
	void callModuleFunction(ModuleFunctions function ,const std::string &message);

	/**
	 * @brief Retrieves the last received message from Aeron.
	 * @return A string view of the last message received.
	 */
	std::string_view getMessage() const;

private:
	bool waitForAeronResponse();
	aeron::fragment_handler_t handleMessage();
	std::string moduleFunctionToCode(ModuleFunctions function) const;

	aeron::Context aeronContext_ {};
	std::shared_ptr<aeron::Aeron> aeron_ {nullptr};
	std::shared_ptr<aeron::Publication> aeronPublication_ {nullptr};
	std::shared_ptr<aeron::Subscription> aeronSubscription_ {nullptr};
	std::shared_ptr<aeron::FragmentAssembler> aeronFragmentAssembler_ {nullptr};
	std::shared_ptr<aeron::fragment_handler_t> aeronHandler_ {nullptr};
	std::shared_ptr<aeron::YieldingIdleStrategy> aeronIdleStrategy_ {nullptr};
	/// Indicates if the Aeron client is currently polling for messages
	std::atomic<bool> aeronPolling_ {false};
	/// Last message received from Aeron
	std::string aeronMessage_ {};

};

}
