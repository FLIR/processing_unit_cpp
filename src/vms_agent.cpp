#include "vms_agent.hpp"
#include "osprey_ws_protocol.hpp"

// #include <jsonconfig.hpp> // json
#include <msgpack.hpp>

#include <iostream>
#include <memory>
#include <cassert>
#include <fstream>

#include "spdlog/spdlog.h"

using namespace std;
using std::chrono::high_resolution_clock;

#define DISPLAY_OUTPUTS 1

namespace ProcessingUnit
{

std::mutex mutex;

void LogTraceVMS(const std::string &Message)
{
    mutex.lock();
    spdlog::get("MainLogger")->trace(Message);
    //std::cout << Message << std::endl;
    //File << Message << std::endl;
    //File.flush();
    mutex.unlock();
}

void LogTraceVMS(const std::string &Message, shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> connection)
{
    mutex.lock();
    std::ostringstream Str;
    Str << connection.get() << " : " << Message;
    spdlog::get("MainLogger")->trace(Str.str());
    // std::cout << Str.str() << std::endl;
    // File << Message << std::endl;
    // File.flush();
    mutex.unlock();
}

bool PortInUse(unsigned short port)
{
    using namespace boost::asio;
    using ip::tcp;

    io_service svc;
    tcp::acceptor a(svc);

    boost::system::error_code ec;
    a.open(tcp::v4(), ec) || a.bind({tcp::v4(), port}, ec);

    return ec == error::address_in_use;
}

std::string ToStringVMS(int Nr)
{
    stringstream ss;
    ss << Nr;
    return ss.str();
}

VmsAgent::~VmsAgent()
{
    _server.stop();
}

void VmsAgent::Reset()
{
}

//------------------------------------------------------------------------------------------------------------------
// VmsAgent
//------------------------------------------------------------------------------------------------------------------
bool VmsAgent::start(
    const string &host,
    int port,
    const string &endpoint_reg_ex)
{
    // Some security checks around the port
    if (PortInUse(port))
    {
        spdlog::get("MainLogger")->error("Server couldn't be started, could it be the port is already in use?");
        return false;
    }

    Reset();

    _server.config.address = host;
    _server.config.port = port;
    //bool TeardownInitiated = false;
    auto &endpoint = _server.endpoint[endpoint_reg_ex];

    // Handle new connection
    endpoint.on_open = [this](shared_ptr<WsServer::Connection> connection) {
        //spdlog::get("MainLogger")->trace("Opened connection " + std::string(connection));
        LogTraceVMS(std::string("Opened connection "), connection);
        m_ConnectionManager.OnOpen(connection);
    };

    // Handle incoming message
    endpoint.on_message = [&](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        LogTraceVMS(std::string(" OnMessage "));

        try
        {
            m_ConnectionManager.OnMessage(connection, std::move(message));
        }
        catch (exception &e)
        {
            spdlog::get("MainLogger")->error(e.what() + std::string(" (closing websocket with message)"));
            connection->send_close(1, e.what(), [&](const SimpleWeb::error_code &ec) {
                spdlog::get("MainLogger")->error(std::string("Server unable to handle the incoming message, Original error message:" + ec.message()));
                m_ConnectionManager.OnClose(connection);
            });
            return;
        }
    };

    // Handle closed connection
    endpoint.on_close = [this](shared_ptr<WsServer::Connection> Connection, int status, const string &Reason) {
        // See RFC 6455 7.4.1. for status codes
        LogTraceVMS(std::string("Closed connection ") + std::string(" with status code ") + ToStringVMS(status), Connection);

        if (Reason.length() > 0)
        {
            LogTraceVMS("Reason for closing: " + Reason, Connection);
        }

        m_ConnectionManager.OnClose(Connection);
        Connection->send_close(1000);

        // Do not stop the server since we want to run as a real server now
        // TODO: need to make this a bit safer in a min
        //Reset();
    };

    // Handle error
    endpoint.on_error = [this](shared_ptr<WsServer::Connection> Connection, const SimpleWeb::error_code &ec) {
        // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
        std::ostringstream ErrStream;
        ErrStream << "Connection error: " << ec.message() << "(" << Connection << ")";
        spdlog::get("MainLogger")->error(ErrStream.str());

        m_ConnectionManager.OnClose(Connection);
        Reset();
    };

    try
    {
        _server.start();
    }
    catch (std::exception &Exc)
    {
        spdlog::get("MainLogger")->error(std::string("Server couldn't be started, could it be the port is already in use? Original error: ") + Exc.what());
    }
    catch (...)
    {
        spdlog::get("MainLogger")->error(std::string("Server couldn't be started, could it be the port is already in use? "));
    }

    return true;
}

}