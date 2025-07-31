#pragma once

#include <Aeron.h>
#include <FragmentAssembler.h>
#include <concurrent/YieldingIdleStrategy.h>



namespace bringauto::aeron_communication {

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

	AeronClient();

	~AeronClient() = default;

	void addModule(uint16_t moduleId);

	void callModuleFunction(ModuleFunctions function ,const std::string &message);

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
	std::atomic<bool> aeronPolling_ {false};
	std::string aeronMessage_ {};

};

}
