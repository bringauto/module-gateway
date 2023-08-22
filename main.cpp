
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
	bas::ModuleLibrary moduleLibrary {};
	try {
		baset::SettingsParser settingsParser;
		if(!settingsParser.parseSettings(argc, argv)) {
			return 0;
		}
		context->settings = settingsParser.getSettings();
		initLogger(context->settings->logPath, context->settings->verbose);
		moduleLibrary.loadLibraries(context->settings->modulePaths);
		moduleLibrary.initStatusAggregators(context);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during initialization: " << e.what() << std::endl;
		return 1;
	}

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	auto toInternalQueue = std::make_shared<bas::AtomicQueue<InternalProtocol::InternalServer >>();
	auto fromInternalQueue = std::make_shared<bas::AtomicQueue<bas::InternalClientMessage >>();
	auto toExternalQueue = std::make_shared<bas::AtomicQueue<InternalProtocol::InternalClient >>();

	bais::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };
	bringauto::modules::ModuleHandler moduleHandler { context, moduleLibrary, fromInternalQueue, toInternalQueue,
													  toExternalQueue };
	bringauto::external_client::ExternalClient externalClient { context, moduleLibrary, toExternalQueue };

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.run(); });
	std::jthread externalClientThread([&externalClient]() { externalClient.run(); });
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.run();

	contextThread.join();
	internalServer.destroy();
	moduleHandler.destroy();
	externalClient.destroy();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}