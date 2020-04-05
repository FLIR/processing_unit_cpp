#ifndef _JOB_CONNECTION_H_
#define _JOB_CONNECTION_H_

#include <server_ws.hpp>
#include <queue>
#include <condition_variable>
#include <memory>
#include <functional>

#include "observable.hpp"
#include "observables_resolver.hpp"
#include "osprey_ws_protocol.hpp"
#include "concurrent_queue.hpp"
#include "job.hpp"
#include "json/jsonconfig.hpp"


using json = nlohmann::json;

namespace ProcessingUnit
{

class Job;

enum class ConnectionState
{
    socket_opened,
    job_started,
    job_started_end_received,
    job_work_finished,
    job_ended,
    error
};
typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef std::shared_ptr<WsServer::Connection> ConnectionPtr;

struct JobInfo
{
    JobInfo() {}
    ConnectionState state;
    ConnectionPtr connection;
    std::string jobId;

    bool IsProcessing() { return false; }
};

class JobConnection
{
public:
    JobConnection();
    ~JobConnection();

    void Init(ConnectionPtr Conn);

    ConnectionState GetState() const { return m_Info.state; }
    void SetState(ConnectionState state) { m_Info.state = state; }
    //
    std::string GetJobId() const { return m_Info.jobId; }

    void NotifyInputData(ObserverDataMessage &input_message) const
    {
        return _input_observable->notify(input_message);
    }

    uint32_t SubscribeProcessorResult(std::function<void(ObserverDataMessage &)> callback) const
    {
        return _processor_result_observable->subscribe(callback);
    }

    void UnsubscribeProcessorResult(uint32_t callback_identifier) const
    {
        _processor_result_observable->unsubscribe(callback_identifier);
    }

    void SetJobId(const std::string &jobId) { m_Info.jobId = jobId; }

    void OnMessage(std::shared_ptr<WsServer::Message> Message);

    // Comunication with Job functions
    void SendContinue();
    void SendData(std::vector<char> &data);

private:
    JobInfo m_Info;
    std::shared_ptr<Job> m_Job;
    std::queue<std::unique_ptr<Message>> m_InputMessages;
    std::queue<std::unique_ptr<Message>> m_OutputMessages;
    bool m_Valid;

    std::shared_ptr<ObservablesResolver> observables_resolver;
    std::shared_ptr<IObservable> _input_observable = observables_resolver->getInputObservable();
    std::shared_ptr<IObservable> _processor_result_observable = observables_resolver->getProcessorResultObservable();

    void Input(ConnectionPtr Conn);
    void Output(ConnectionPtr Conn);

    std::mutex m_DataProtector;
    std::mutex m_MutexContinue;
    std::mutex m_DataProtectorInputQueue;
    std::mutex m_DataProtectorOutputQueue;
    std::condition_variable m_ConVarOutputQueue;
    std::condition_variable m_ConVarInputQueue;
    bool m_Processing;
    bool m_isContinueMessageRecived = true;
    bool m_isEndMessageReceived = false;
    bool m_isJobStoped = false;

    bool m_isOutputQueueEmpty = true;
    bool m_isInputQueueEmpty = true;

    void HandleStartMessage(std::unique_ptr<StartMessage> Msg);
    void HandleReadyMessage();
    void HandleMessage(std::unique_ptr<Message> Msg);
    void HandleContinueMessage();
    void HandleEndMessage();

    void ProcessData(std::unique_ptr<DataMessage> Msg);
};
} // namespace ProcessingUnit
#endif // _JOB_CONNECTION_H_
