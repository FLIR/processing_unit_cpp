#ifdef WIN32
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#endif
#include "processing_unit_server.hpp"

#include <msgpack.hpp>
#include <string>
#include <iostream>
#include <sstream>

#include <vms_agent.hpp>

#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <future>

// logging
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "json/jsonconfig.hpp"

namespace ProcessingUnit
{
const std::string NameLogger("MainLogger");

bool ProcessingUnitServer::StartProcessingUnitServer()
{
    // Setup logger
    auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    ConsoleSink->set_level(spdlog::level::trace); // only shows warnings or errors
    ConsoleSink->set_pattern("[%H:%M:%S] [%^%l%$] [thread %t] - %v ");

    auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
    FileSink->set_level(spdlog::level::trace);
    FileSink->set_pattern("[%H:%M:%S] [%^%l%$] [thread %t] - %v ");

    auto console = spdlog::stderr_color_mt("console");
    auto err = spdlog::stderr_color_mt("error");
    //"MainLogger", { ConsoleSink, FileSink }
    std::vector<spdlog::sink_ptr> Sinks { ConsoleSink, FileSink };
    std::shared_ptr<spdlog::logger> Logger = std::make_shared<spdlog::logger>(NameLogger, Sinks.begin(), Sinks.end());
    spdlog::register_logger(Logger);
    spdlog::get(NameLogger)->set_level(spdlog::level::trace);

	//-------------------
	// Main program loop
	//-------------------
	std::ostringstream Stream;
	Stream << "Opening websocket server - " << "  -> " << _host << ":" << _port << std::endl;
	spdlog::get(NameLogger)->info(Stream.str());

	// Attach our callbacks to the agent and start it up. Ctrl+C to exit.
	bool success = VmsAgent().start(_host, _port);
	if (!success) {
		spdlog::get(NameLogger)->error("Failed to start agent");
		return false;
	}

	return true;
}
}