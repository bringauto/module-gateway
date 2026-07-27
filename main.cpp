
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

#include <bringauto/quic/Logger.hpp>

#include <InternalProtocol.pb.h>
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
		bringauto::quic::Logger::addSink<bringauto::logging::ConsoleSink>(paramConsoleSink);
	}
	if(settings.file.use) {
		bringauto::logging::FileSink::Params paramFileSink { settings.file.path, "ModuleGateway.log" };
		using namespace bringauto::logging;
		paramFileSink.maxFileSize = 50_MiB;
		paramFileSink.numberOfRotatedFiles = 5;
		paramFileSink.verbosity = settings.file.level;
		bringauto::settings::Logger::addSink<FileSink>(paramFileSink);
		bringauto::quic::Logger::addSink<FileSink>(paramFileSink);
	}

	bringauto::settings::Logger::init("ModuleGateway");
	// ba-quic-lib uses its own separate logger instance (LoggerId "ba-quic-lib") --
	// without this init() call its internal trace/warning logs are silently swallowed.
	bringauto::quic::Logger::init({std::string(bringauto::quic::kLoggerId.id),
	                               bringauto::settings::toLoggerVerbosity(
	                                       BRINGAUTO_MODULE_GATEWAY_MINIMUM_LOGGER_VERBOSITY)});
}

int main(int argc, char **argv) {
	namespace bais = bringauto::internal_server;
	namespace bas = bringauto::structures;
	namespace baset = bringauto::settings;

	baset::Settings settings {};
	try {
		baset::SettingsParser settingsParser;
		if(!settingsParser.parseSettings(argc, argv)) {
			return 0;
		}
		settings = settingsParser.getSettings();
		initLogger(settings.loggingSettings);
		baset::Logger::logInfo("Version: {}", MODULE_GATEWAY_VERSION);
		baset::Logger::logInfo("Loaded config:\n{}", settingsParser.serializeToJson());
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during reading configuration: " << e.what() << std::endl;
		return 1;
	}

	bas::GlobalContext context { std::move(settings) };

	bas::ModuleLibrary moduleLibrary {};

	try {
		if(context.settings.moduleBinaryPath.empty()) {
			moduleLibrary.loadLibraries(context.settings.modulePaths);
		} else {
			moduleLibrary.loadLibraries(context.settings.modulePaths, context.settings.moduleBinaryPath);
		}
		moduleLibrary.initStatusAggregators(context);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during module initialization: " << e.what() << std::endl;
		return 1;
	}

	bas::AtomicQueue<bas::ModuleHandlerMessage> toInternalQueue;
	bas::AtomicQueue<bas::InternalClientMessage> fromInternalQueue;
	bas::AtomicQueue<bas::InternalClientMessage> commandForwardingQueue;
	bas::AtomicQueue<bas::InternalClientMessage> toExternalQueue;

	bais::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };
	bringauto::modules::ModuleHandler moduleHandler { context, moduleLibrary, fromInternalQueue,
													  commandForwardingQueue, toInternalQueue, toExternalQueue };
	bringauto::external_client::ExternalClient externalClient { context, moduleLibrary, toExternalQueue, commandForwardingQueue };

	// Stop the io_context and wake every queue wait so all worker threads exit promptly
	auto requestShutdown = [&]() {
		context.ioContext.stop();
		toInternalQueue.interrupt();
		fromInternalQueue.interrupt();
		commandForwardingQueue.interrupt();
		toExternalQueue.interrupt();
		externalClient.requestStop();
	};

	boost::asio::signal_set signals(context.ioContext, SIGINT, SIGTERM);
	signals.async_wait([&](auto, auto) { requestShutdown(); });

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.run(); });
	std::jthread externalClientThread([&externalClient]() { externalClient.run(); });
	std::jthread contextThread2([&context]() { context.ioContext.run(); });
	std::jthread contextThread1([&context]() { context.ioContext.run(); });
	try {
		internalServer.run();
	} catch(boost::system::system_error &e) {
		baset::Logger::logError("Error during run {}", e.what());
		requestShutdown();
	}

	contextThread2.join();
	contextThread1.join();
	externalClientThread.join();
	moduleHandlerThread.join();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
