#include "job_connection_manager.hpp"
#include <string>
#include <utility>
#include <fstream>

#include "spdlog/spdlog.h"

namespace ProcessingUnit
{
void JobConnectionManager::OnOpen(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    m_Jobs[conn].Init(conn);
}

void JobConnectionManager::OnMessage(ConnectionPtr conn, std::shared_ptr<WsServer::Message> Message)
{
    std::ostringstream Str;
    Str << "OnMes " << conn << " ",

        spdlog::get("MainLogger")->trace(Str.str() + " Start");
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    if (m_Jobs.count(conn))
    {
        std::cout << int(m_Jobs[conn].GetState()) << std::endl;
        m_Jobs[conn].OnMessage(Message);
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Could not find job for sending message " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
    }
    spdlog::get("MainLogger")->trace(Str.str() + " End");
}

void JobConnectionManager::OnClose(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    if (m_Jobs.count(conn))
    {
        m_Jobs.erase(conn); // PIM : todo idiom erase/remove
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Could not find job for sending message " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
    }
}

ConnectionState JobConnectionManager::GetConnectionState(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    if (m_Jobs.count(conn))
    {
        return m_Jobs[conn].GetState(); // PIM : todo idiom erase/remove
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Couldn't retrieve the state for a job " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
        return ConnectionState::error;
    }
}

void JobConnectionManager::ChangeConnectionState(ConnectionPtr conn, ConnectionState state)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    if (m_Jobs.count(conn))
    {
        return m_Jobs[conn].SetState(state); // PIM : todo idiom erase/remove
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Couldn't change the state for a job " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
    }
}

std::string JobConnectionManager::GetJobId(ConnectionPtr conn)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex);
    if (m_Jobs.count(conn))
    {
        return m_Jobs[conn].GetJobId(); // PIM : todo idiom erase/remove
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Couldn't retrieve the jobid for a job " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
        return "Error";
    }
}

void JobConnectionManager::AddJobId(ConnectionPtr conn, const std::string &jobId)
{
    std::lock_guard<std::mutex> lock(m_ConnectionsMutex); // not strictly needed
    if (m_Jobs.count(conn))
    {
        return m_Jobs[conn].SetJobId(jobId); // PIM : todo idiom erase/remove
    }
    else
    {
        std::ostringstream ErrStr;
        ErrStr << "JobConnectionManager: Couldn't add a jobId for job " << conn;
        spdlog::get("MainLogger")->error(ErrStr.str());
    }
}
} // namespace ProcessingUnit