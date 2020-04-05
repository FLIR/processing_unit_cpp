#ifndef _PROCESSING_UNIT_HPP
#define _PROCESSING_UNIT_HPP
#include <string>
#include <queue>
#include <msgpack.hpp>
#include <iostream>


namespace ProcessingUnit
{
class Websocket;

class ProcessingUnitServer
{
public:
	ProcessingUnitServer(const int port)
		: _host(""), _port(port){};

	bool StartProcessingUnitServer();

private:
	const std::string _host;
	const int _port;
};

} // namespace ProcessingUnit
#endif
