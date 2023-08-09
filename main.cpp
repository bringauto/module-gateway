
#include <bringauto/internal_server/InternalServer.hpp>
#include <bringauto/structures/GlobalContext.hpp>
#include <bringauto/structures/ModuleLibrary.hpp>
#include <bringauto/external_client/ExternalClient.hpp>
#include <bringauto/modules/ModuleHandler.hpp>
#include <bringauto/structures/AtomicQueue.hpp>
#include <bringauto/settings/SettingsParser.hpp>
#include <bringauto/utils/utils.hpp>
#include <InternalProtocol.pb.h>

#include <thread>



int main(int argc, char **argv) {
	namespace bais = bringauto::internal_server;
	namespace bas = bringauto::structures;
	namespace baset = bringauto::settings;
	auto context = std::make_shared<bas::GlobalContext>();
    bas::ModuleLibrary moduleLibrary{};
	try {
		baset::SettingsParser settingsParser;
		if(!settingsParser.parseSettings(argc, argv)) {
			return 0;
		}
		context->settings = settingsParser.getSettings();
		bringauto::utils::initLogger(context->settings->logPath, context->settings->verbose);
        moduleLibrary.loadLibraries(context->settings->modulePaths);
        moduleLibrary.initStatusAggregators(context);
	} catch(std::exception &e) {
		std::cerr << "[ERROR] Error occurred during initialization: " << e.what() << std::endl;
		return 1;
	}

	boost::asio::signal_set signals(context->ioContext, SIGINT, SIGTERM);
	signals.async_wait([context](auto, auto) { context->ioContext.stop(); });

	auto toInternalQueue = std::make_shared < bas::AtomicQueue < InternalProtocol::InternalServer >> ();
	auto fromInternalQueue = std::make_shared < bas::AtomicQueue < InternalProtocol::InternalClient >> ();
	auto toExternalQueue = std::make_shared < bas::AtomicQueue < InternalProtocol::InternalClient >> ();

	bais::InternalServer internalServer { context, fromInternalQueue, toInternalQueue };
	bringauto::modules::ModuleHandler moduleHandler { context, moduleLibrary, fromInternalQueue, toInternalQueue, toExternalQueue };
	bringauto::external_client::ExternalClient externalClient { context, moduleLibrary, toExternalQueue };

	std::jthread moduleHandlerThread([&moduleHandler]() { moduleHandler.run(); });
	std::jthread externalClientThread([&externalClient]() { externalClient.run(); });
	std::jthread contextThread([&context]() { context->ioContext.run(); });
	internalServer.start();

	contextThread.join();
	internalServer.stop();
	moduleHandler.destroy();
	externalClient.destroy();

    google::protobuf::ShutdownProtobufLibrary();

	return 0;
}