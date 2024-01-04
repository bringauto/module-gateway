
#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/logging/Logger.hpp>

#include <InternalProtocol.pb.h>
#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>

#include <thread>



void initLogger(const std::string &logPath, bool verbose) {
	using namespace bringauto::logging;
	if(verbose) {
		Logger::addSink<bringauto::logging::ConsoleSink>();
	}
	FileSink::Params paramFileSink { logPath, "ModuleGateway.log" };
	paramFileSink.maxFileSize = 50_MiB;
	paramFileSink.numberOfRotatedFiles = 5;
	paramFileSink.verbosity = Logger::Verbosity::Info;

	Logger::addSink<bringauto::logging::FileSink>(paramFileSink);
	Logger::LoggerSettings params { "ModuleGateway",
									Logger::Verbosity::Debug }; // TODO change to Info
	Logger::init(params);
}

int main(int argc, char **argv) {
	namespace bais = bringauto::internal_server;
	namespace bas = bringauto::structures;
	namespace baset = bringauto::settings;
	auto context = std::make_shared<bas::GlobalContext>();
	try {
		baset::SettingsParser settingsParser;
		if(!settingsParser.parseSettings(argc, argv)) {
			return 0;
		}
		context->settings = settingsParser.getSettings();
		initLogger(context->settings->logPath, context->settings->verbose);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during reading configuration: " << e.what() << std::endl;
		return 1;
	}
	bas::ModuleLibrary moduleLibrary {};
	try {
		moduleLibrary.loadLibraries(context->settings->modulePaths);
		moduleLibrary.initStatusAggregators(context);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during module initialization: " << e.what() << std::endl;
		return 1;
	}

	std::jthread moduleHandlerThread;
	std::jthread externalClientThread;
	std::jthread contextThread;
	std::unique_ptr<bais::InternalServer> internalServer;
	std::unique_ptr<bringauto::modules::ModuleHandler> moduleHandler;
	std::unique_ptr<bringauto::external_client::ExternalClient> externalClient;
	try {
		boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
		signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

		auto toInternalQueue = std::make_shared<bas::AtomicQueue<InternalProtocol::InternalServer >>();
		auto fromInternalQueue = std::make_shared<bas::AtomicQueue<bas::InternalClientMessage >>();
		auto toExternalQueue = std::make_shared<bas::AtomicQueue<bas::InternalClientMessage >>();

		internalServer = std::make_unique<bais::InternalServer>(context, fromInternalQueue, toInternalQueue);
		moduleHandler = std::make_unique<bringauto::modules::ModuleHandler>(context, moduleLibrary, fromInternalQueue,
																			toInternalQueue,
																			toExternalQueue);
		externalClient = std::make_unique<bringauto::external_client::ExternalClient>(context, moduleLibrary,
																					 toExternalQueue);

		moduleHandlerThread = std::jthread([&moduleHandler]() { moduleHandler->run(); });
		externalClientThread = std::jthread([&externalClient]() { externalClient->run(); });
		contextThread = std::jthread([&context]() { context->ioContext.run(); });
		internalServer->run();
	} catch(boost::system::system_error &e) {
		bringauto::logging::Logger::logError("Error during run {}", e.what());
		context->ioContext.stop();
	}

	contextThread.join();
	externalClientThread.join();
	moduleHandlerThread.join();

	internalServer->destroy();
	moduleHandler->destroy();
	externalClient->destroy();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}