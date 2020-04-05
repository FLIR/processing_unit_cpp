#ifndef _JOB_CONNECTION_MANAGER_H_
#define _JOB_CONNECTION_MANAGER_H_

#include <server_ws.hpp>
#include <queue>
#include <condition_variable>
#include "osprey_ws_protocol.hpp"
#include "job_connection.hpp"

namespace ProcessingUnit
{
class JobConnectionManager
{
private:
    typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
    typedef std::shared_ptr<WsServer::Connection> ConnectionPtr;

public:
    void OnOpen(ConnectionPtr conn);
    void AddJobIdToConnection(ConnectionPtr conn, const std::string &jobId);
    void AddInputMessage(ConnectionPtr conn, std::unique_ptr<DataMessage> msg);
    void AddOutputMessage(ConnectionPtr conn, std::unique_ptr<DataMessage> msg);

    void OnMessage(ConnectionPtr conn, std::shared_ptr<WsServer::Message> message);
    void OnClose(ConnectionPtr conn);
    bool IsEmpty() const { return m_Jobs.empty(); }

    ConnectionState GetConnectionState(ConnectionPtr conn);
    void ChangeConnectionState(ConnectionPtr conn, ConnectionState state);
    std::string GetJobId(ConnectionPtr conn);
    void AddJobId(ConnectionPtr conn, const std::string &jobId);

private:
    std::map<ConnectionPtr, JobConnection> m_Jobs;
    std::mutex m_ConnectionsMutex;
};
} // namespace ProcessingUnit
#endif // _JOB_CONNECTION_MANAGER_H_
