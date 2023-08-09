#include <bringauto/utils/utils.hpp>

#include <bringauto/modules/StatusAggregator.hpp>
#include <bringauto/logging/Logger.hpp>

#include <libbringauto_logger/bringauto/logging/Logger.hpp>
#include <libbringauto_logger/bringauto/logging/FileSink.hpp>
#include <libbringauto_logger/bringauto/logging/ConsoleSink.hpp>



namespace bringauto::utils {

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

std::vector <std::string> splitString(const std::string &input, char delimiter) {
	std::vector <std::string> tokens;
	std::istringstream iss(input);
	std::string token;

	while(std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

std::string getId(const ::device_identification &device) {
	std::stringstream ss;
	ss << device.module << "/" << device.device_type << "/" << std::string{static_cast<char *>(device.device_role.data), device.device_role.size_in_bytes} << "/"
	   << std::string{static_cast<char *>(device.device_name.data), device.device_name.size_in_bytes}; // TODO we need to be able to get priority
	return ss.str();
}

void initBuffer(struct buffer &buffer, const std::string &data){
    allocate(&buffer, data.size());
	std::memcpy(buffer.data, data.c_str(), data.size());
}

void deallocateDeviceId(struct ::device_identification &device){
    deallocate(&device.device_role);
    deallocate(&device.device_name);
}

}