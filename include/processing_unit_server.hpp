#ifndef _PROCESSING_UNIT_HPP
#define _PROCESSING_UNIT_HPP
#include "vms_agent.hpp"
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
		:vmsAgent(nullptr), _host(""), _port(port){};
	virtual ~ProcessingUnitServer()
	{
		if(vmsAgent)
		{
			delete vmsAgent;
			vmsAgent = nullptr;
		} 
	};

	bool StartProcessingUnitServer();
	void StopProcessingUnitServer();

private:
	VmsAgent *vmsAgent;
	const std::string _host;
	const int _port;
};

} // namespace ProcessingUnit
#endif
