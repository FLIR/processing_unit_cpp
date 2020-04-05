#ifndef _VMS_AGENT_HPP
#define _VMS_AGENT_HPP

#include <server_ws.hpp>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "osprey_ws_protocol.hpp"
#include "job_connection_manager.hpp"
#include "json/jsonconfig.hpp"

namespace ProcessingUnit
{
  class VmsAgent
  {
    public:
      VmsAgent() = default;
      ~VmsAgent();
      VmsAgent& operator=(const VmsAgent&) = delete; // Disallow copying
      VmsAgent(const VmsAgent&) = delete; // Disallow copying

      bool start(
          const std::string& host,
          int port,
          const std::string& endpoint_reg_ex = "^/$"
          );

    private:
      enum class ConnectionState { socket_opened, job_started, job_started_end_received, job_work_finished, job_ended, error };
      typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
      typedef std::shared_ptr<WsServer::Connection> ConnectionPtr;


      WsServer _server; // The websockets server
      std::map<ConnectionPtr, ConnectionState> connection_states; // Track state of connections
      std::map<ConnectionPtr, std::string> connection_job_ids; // Map connections to jobIds

      JobConnectionManager m_ConnectionManager;

      bool m_Processing;
      std::queue<std::unique_ptr<Message>> m_InputMessages;
      std::queue<std::unique_ptr<Message>> m_OutputMessages;

      std::mutex m_DataProtector;

      bool m_ReceivedOutputContinue;
      std::condition_variable m_output_continue;
      std::mutex m_output_continue_mutex;

      void Reset();

      std::function<bool(const std::string&, const json&, std::string&)> m_InitCb; //!< Init callback : job initialization callback
      std::function<json(const std::string&, const std::vector<char>& data, std::vector<std::unique_ptr<DataMessage>>& Results)> m_DataCb; //!< Data callback : job data callback

      void Input(std::shared_ptr<WsServer::Connection> Conn);
      void Output(std::shared_ptr<WsServer::Connection> Conn);

      void HandleStartMessage(std::unique_ptr<StartMessage> Msg, std::shared_ptr<WsServer::Connection>& Conn);
      void HandleReadyMessage(std::unique_ptr<ReadyMessage> Msg);
      void HandleDataMessage(std::unique_ptr<DataMessage> Msg, std::shared_ptr<WsServer::Connection>& Conn);
      void HandleContinueMessage(std::unique_ptr<ContinueMessage> Msg, std::shared_ptr<WsServer::Connection>& Conn);
      void HandleEndMessage(std::unique_ptr<EndMessage> Msg, std::shared_ptr<WsServer::Connection>& Conn);
      void ProcessData(std::unique_ptr<DataMessage> Msg, std::shared_ptr<WsServer::Connection>& Conn);

  };
}

#endif

