
#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/structures/InternalClientMessage.hpp>
#include <bringauto/structures/ModuleHandlerMessage.hpp>
#include <bringauto/settings/LoggerId.hpp>

#include <InternalProtocol.pb.h>
#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>

#include <thread>

#ifndef MODULE_GATEWAY_VERSION
#define MODULE_GATEWAY_VERSION "VERSION_NOT_SET"
#endif


void initLogger(const bringauto::structures::LoggingSettings &settings) {
	if(settings.console.use) {
		bringauto::logging::ConsoleSink::Params paramConsoleSink { settings.console.level };
		bringauto::settings::Logger::addSink<bringauto::logging::ConsoleSink>(paramConsoleSink);
	}
	if(settings.file.use) {
		bringauto::logging::FileSink::Params paramFileSink { settings.file.path, "ModuleGateway.log" };
		using namespace bringauto::logging;
		paramFileSink.maxFileSize = 50_MiB;
		paramFileSink.numberOfRotatedFiles = 5;
		paramFileSink.verbosity = settings.file.level;
		bringauto::settings::Logger::addSink<FileSink>(paramFileSink);
	}

	bringauto::settings::Logger::init("ModuleGateway");
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
		initLogger(context->settings->loggingSettings);
		baset::Logger::logInfo("Version: {}", MODULE_GATEWAY_VERSION);
		baset::Logger::logInfo("Loaded config:\n{}", settingsParser.serializeToJson());
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during reading configuration: " << e.what() << std::endl;
		return 1;
	}

	bas::ModuleLibrary moduleLibrary {};

	try {
		moduleLibrary.loadLibraries(context->settings->modulePaths, context->settings->moduleBinaryPath);
		moduleLibrary.initStatusAggregators(context);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during module initialization: " << e.what() << std::endl;
		return 1;
	}

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	auto toInternalQueue = std::make_shared<bas::AtomicQueue<bas::ModuleHandlerMessage >>();
	auto fromInternalQueue = std::make_shared<bas::AtomicQueue<bas::InternalClientMessage >>();
	auto toExternalQueue = std::make_shared<bas::AtomicQueue<bas::InternalClientMessage >>();

	bais::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };
	bringauto::modules::ModuleHandler moduleHandler { context, moduleLibrary, fromInternalQueue, toInternalQueue,
													  toExternalQueue };
	bringauto::external_client::ExternalClient externalClient { context, moduleLibrary, toExternalQueue };

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.run(); });
	std::jthread externalClientThread([&externalClient]() { externalClient.run(); });
	std::jthread contextThread2([&context]() { context->ioContext.run(); });
	std::jthread contextThread1([&context]() { context->ioContext.run(); });
	try {
		internalServer.run();
	} catch(boost::system::system_error &e) {
		baset::Logger::logError("Error during run {}", e.what());
		context->ioContext.stop();
	}

	contextThread2.join();
	contextThread1.join();
	externalClientThread.join();
	moduleHandlerThread.join();

	internalServer.destroy();
	moduleHandler.destroy();
	externalClient.destroy();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
