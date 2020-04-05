#include "job_connection.hpp"
#include "osprey_ws_protocol.hpp"
#include <thread>
#include <stdexcept>
#include <sstream>

#include "spdlog/spdlog.h"

#include "job.hpp"

namespace ProcessingUnit
{

std::mutex mu;

void LogTrace(const std::string &Message)
{
    mu.lock();
    spdlog::get("MainLogger")->trace(Message);
    mu.unlock();
}

void LogInfo(const std::string &Message)
{
    mu.lock();
    spdlog::get("MainLogger")->info(Message);
    mu.unlock();
}

void LogTrace(const std::string &Message, std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> connection)
{
    mu.lock();
    std::ostringstream Str;
    Str << connection.get() << " : " << Message;
    spdlog::get("MainLogger")->trace(Str.str());
    mu.unlock();
}

void LogInfo(const std::string &Message, std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> connection)
{
    mu.lock();
    std::ostringstream Str;
    Str << connection.get() << " : " << Message;
    spdlog::get("MainLogger")->info(Str.str());
    mu.unlock();
}

void chk_throw(bool condition, const std::string &message, const std::string &prefix = "")
{
    if (!condition)
    {
        throw std::runtime_error(std::string(prefix + message));
    }
}

/*!
    Use below when we're certain that the derived class is of a certain type
*/
template <typename Derived, typename Base, typename Del>
std::unique_ptr<Derived, Del>
static_unique_ptr_cast(std::unique_ptr<Base, Del> &&p)
{
    auto d = static_cast<Derived *>(p.release());
    return std::unique_ptr<Derived, Del>(d, std::move(p.get_deleter()));
}

template <typename D, typename B>
std::unique_ptr<D> static_cast_ptr(std::unique_ptr<B> &base)
{
    return std::unique_ptr<D>(static_cast<D *>(base.release()));
}

std::string ToString(int Nr)
{
    std::stringstream ss;
    ss << Nr;
    return ss.str();
}

template <class CertainMessageType>
void SendMessage(std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> &Conn,
                 CertainMessageType &Msg)
{
    auto SendStream = std::make_shared<SimpleWeb::SocketServer<SimpleWeb::WS>::SendStream>();
    msgpack::pack(*SendStream, Msg);

    Conn->send(
        SendStream, [&](const SimpleWeb::error_code &Err) {
            if (Err)
            {
                std::ostringstream ErrStr;
                ErrStr << "Error sending message: " << Err << " - " << Msg.GetMessageTypeAsString();
                spdlog::get("MainLogger")->error(ErrStr.str());
            }
        },
        130);
}

JobConnection::JobConnection()
    : m_Valid(false)
{
}

JobConnection::~JobConnection()
{
    m_Processing = false;

    if (m_OutputMessages.size())
    {
        spdlog::get("MainLogger")->warn("Not all output messages were sent");
    }

    if (m_InputMessages.size())
    {
        spdlog::get("MainLogger")->warn("Not all input messages were processed");
    }
    spdlog::get("MainLogger")->info("Destroying job & connection");
}

void JobConnection::Init(ConnectionPtr Conn)
{
    m_Info.connection = Conn;
    m_Valid = true;
    m_Processing = true;
    SetState(ConnectionState::socket_opened);

    std::thread InputThreadOrg([this, Conn] { Input(Conn); });
    std::thread OutputThreadOrg([this, Conn] { Output(Conn); });

    InputThreadOrg.detach();
    OutputThreadOrg.detach();

    // std::shared_ptr<cortex::NNTCPublisherResolver> nntc_pub_resolver;
    // _input_pub = nntc_pub_resolver->getInputPublisher();
    // _nntc_va_report_pub = nntc_pub_resolver->getVAReportPublisher();
}

void JobConnection::Input(ConnectionPtr Conn)
{
    LogTrace("#Input thread created", Conn);
    while (m_Processing)
    {
        LogTrace("#Input acq lock", Conn);
        std::unique_lock<std::mutex> input_queue_lock(m_DataProtectorInputQueue);
        m_ConVarInputQueue.wait(input_queue_lock, [&] { return !m_isInputQueueEmpty; });
        std::unique_ptr<Message> Msg(std::move(m_InputMessages.front()));
        m_InputMessages.pop();
        m_isInputQueueEmpty = m_InputMessages.empty();
        //Lock.unlock(); // PIM !!! used to be lower, could have issues -> verify

        LogTrace("#Input msg: " + Msg->GetMessageTypeAsString(), Conn);

        if (Msg->GetMessageType() == Message::Data)
        {
            LogTrace("#Input we would process this data", Conn);
            std::unique_ptr<DataMessage> DataMsg = static_cast_ptr<DataMessage>(Msg); //!TOOD: does this point to bad design? Downcasting here
            ProcessData(std::move(DataMsg));
        }
        else if (Msg->GetMessageType() == Message::End)
        {
            //  LogTrace("#Input we would add this end message to the output queue", Conn);
            m_Job->stopJob();
            std::unique_lock<std::mutex> output_queue_lock(m_DataProtectorOutputQueue);
            m_isJobStoped = true;
            output_queue_lock.unlock();
            m_ConVarOutputQueue.notify_one();
        }

        input_queue_lock.unlock();
    }
    LogTrace("#Input thread destroyed", Conn);
}

void JobConnection::Output(ConnectionPtr Conn)
{
    while (m_Processing)
    {
        LogTrace("#Output called...locking", Conn);
        std::unique_lock<std::mutex> output_queue_lock(m_DataProtectorOutputQueue);
        auto condition_var_predicate = [&] {
            return (m_isJobStoped && m_isOutputQueueEmpty) || (m_isContinueMessageRecived && !m_isOutputQueueEmpty);
        };
        m_ConVarOutputQueue.wait(output_queue_lock, condition_var_predicate);
        std::ostringstream Msg;

        if (m_isContinueMessageRecived && !m_isOutputQueueEmpty)
        {
            std::unique_ptr<Message> Msg(std::move(m_OutputMessages.front()));
            m_OutputMessages.pop();
            m_isOutputQueueEmpty = m_OutputMessages.empty();
            LogTrace(std::string("-> ") + Msg->GetMessageTypeAsString() + std::string("(#Output)"), Conn);
            if (Msg->GetMessageType() == Message::Data)
            {
                LogTrace("  #Output data", Conn);
                std::unique_ptr<DataMessage> DataMsg = static_cast_ptr<DataMessage>(Msg);
                SendMessage<DataMessage>(Conn, *DataMsg);
                // m_ReceivedData = false;
            }

            m_isContinueMessageRecived = false;
        }

        if (m_isJobStoped && m_isOutputQueueEmpty)
        {
            LogTrace("  #Output end", Conn);
            EndMessage Msg;
            SendMessage<EndMessage>(Conn, Msg);
            SetState(ConnectionState::job_ended);
            m_Processing = false;
            break;
        }

        output_queue_lock.unlock();
        if (m_isOutputQueueEmpty)
        {
            m_ConVarOutputQueue.notify_one();
        }
        LogTrace("#Output unlocked", Conn);
    }
    LogTrace("#Output destroyed", Conn);
}

void JobConnection::OnMessage(std::shared_ptr<WsServer::Message> Message)
{
    if (m_Info.state == ConnectionState::error)
    {
        throw std::runtime_error("Got message for connection in error state");
    }

    const std::string Bytes = Message->string();
    LogTrace(std::string("Message received: ") + ToString(Bytes.size()), m_Info.connection); // +connection.get());

    MessageReader Reader;
    bool CouldParse = Reader.Parse(Bytes);

    Message::MessageType Type = Reader.GetMessageType();

    switch (Type)
    {
    case Message::Start:
    {
        ConnectionState State = GetState();
        std::cout << int(State) << std::endl;
        chk_throw(GetState() == ConnectionState::socket_opened, "Got start message after initial handshake was complete");
        std::unique_ptr<StartMessage> Msg = Reader.GetStartMessage();
        HandleStartMessage(std::move(Msg));
    }
    break;

    case Message::Ready:
    {
        HandleReadyMessage();
    }
    break;

    case Message::Data:
    {
        chk_throw(GetState() > ConnectionState::socket_opened, "Got data message but job was not started");
        chk_throw(GetJobId() != "", "No job id for this connection");

        std::unique_ptr<DataMessage> Msg = Reader.GetDataMessage();
        HandleMessage(std::move(Msg));
    }
    break;

    case Message::Continue:
    {
        HandleContinueMessage();
    }
    break;

    case Message::End:
    {
        chk_throw(GetState() <= ConnectionState::job_ended, "Got end message but job was not started");
        HandleEndMessage();
    }
    break;

    default:
        LogTrace("*Unknown message received*");
        break;
    }
}

void JobConnection::HandleStartMessage(std::unique_ptr<StartMessage> Msg)
{
    LogInfo("<- *Start received*", m_Info.connection);
    if (Msg->GetMessageType() == Message::Start)
    {
        const std::string JobId = Msg->GetJobId();
        const json Config = Msg->GetInfoJson();
        std::cout << Config << std::endl;
        SetJobId(JobId);

        std::ostringstream JobInfo;
        JobInfo << "-Connection Jobid: " << JobId << " conn: " << m_Info.connection;
        LogTrace(JobInfo.str(), m_Info.connection);

        bool ConfigSuccess = true;
        std::string ErrorMessage;

        m_Job = std::make_shared<Job>(Msg->GetInfoJson(), this);

        SetState(ConnectionState::job_started);

        if (ConfigSuccess)
        {
            //NNTC part
            // auto startData = DataPtr(); //Need to contain the json config of nntc
            //end part
            ReadyMessage RespMsg(true);

            SendMessage<ReadyMessage>(m_Info.connection, RespMsg);
        }
        else
        {
            ReadyMessage RespMsg(false, ErrorMessage);
            SendMessage<ReadyMessage>(m_Info.connection, RespMsg);
        }
    }
}

void JobConnection::HandleReadyMessage()
{
    LogInfo("<- *Ready received*");
    spdlog::get("MainLogger")->warn("We should never receive a ReadyMessage on the processor side !");
}

void JobConnection::HandleMessage(std::unique_ptr<Message> Msg)
{
    LogInfo("<- *Data received*", m_Info.connection);
    // Add message to queue
    std::unique_lock<std::mutex> lck(m_DataProtectorInputQueue);
    m_isInputQueueEmpty = false;
    m_InputMessages.push(std::move(Msg));
    lck.unlock();
    m_ConVarInputQueue.notify_one();
}

void JobConnection::ProcessData(std::unique_ptr<DataMessage> Msg)
{
    LogTrace("-- *Data processing* --", m_Info.connection);

    const std::string Metadata = Msg->GetMetaData();
    // Decode data
    std::vector<char> BufVec = Msg->GetPayloadData();
    // Process data
    std::vector<std::unique_ptr<DataMessage>> Results;

    json Response;

    try
    {
        //auto Response = m_DataCb(GetJobId(), BufVec, Results);
        Response = m_Job->process(BufVec);
    }
    catch (std::exception &Exc)
    {
        std::cout << Exc.what() << std::endl;
    }

    // Enable our output
    LogTrace("-- *Finished processing*", m_Info.connection);
}

void JobConnection::HandleContinueMessage()
{
    LogInfo("<- *Continue received*");
    std::unique_lock<std::mutex> output_queue_lock(m_DataProtectorOutputQueue);
    m_isContinueMessageRecived = true;
    output_queue_lock.unlock();
    m_ConVarOutputQueue.notify_one();
}

void JobConnection::HandleEndMessage()
{
    LogInfo("<- *End received*");
    HandleMessage(std::unique_ptr<EndMessage>(new EndMessage()));
}

void JobConnection::SendContinue()
{
    LogInfo("-> Send Continue message", m_Info.connection);
    ContinueMessage ContinueMsg;
    SendMessage<ContinueMessage>(m_Info.connection, ContinueMsg);
}

void JobConnection::SendData(std::vector<char> &data)
{
    LogTrace("#SendData we would add this data message to the output queue", m_Info.connection);
    std::unique_lock<std::mutex> output_queue_lock(m_DataProtectorOutputQueue);
    m_OutputMessages.push(std::unique_ptr<DataMessage>(new DataMessage("", data)));
    LogInfo("#Output queue size is now:" + std::to_string(m_OutputMessages.size()));
    m_isOutputQueueEmpty = false;
    output_queue_lock.unlock();
    m_ConVarOutputQueue.notify_one();
}
} // namespace ProcessingUnit