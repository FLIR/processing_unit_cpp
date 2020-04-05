#ifndef _JOB_H_
#define _JOB_H_

// PIM: cleanup
#include <memory>
#include <thread>
#include <future>

#include "job_connection.hpp"
#include "json/jsonconfig.hpp"

namespace ProcessingUnit
{
class JobConnection;

class Job
{
public:
    Job(const json &config, JobConnection *job_con);

    ~Job();

    // Add data to the processing queue
    json process(DataPtr &data);

    // Functions for the PU implementation
    bool readData(DataPtr *data);
    void writeData(DataPtr &data);
    void stopJob();
    bool isStoped();

private:
    void Processing();

    JobConnection *m_JobConnection;
    std::queue<DataPtr> m_InputData;
    std::mutex m_DataProtector;
    std::condition_variable m_ConditionVariable;
    volatile bool m_isJobEmpty = true;
    volatile bool m_isStopJobSignaled = false;
    std::thread m_ProcessThread;
    std::condition_variable m_ConVarVARecived;
    volatile bool m_isVARecived = false;
    uint32_t _callback_identifier;
};
} // namespace ProcessingUnit
#endif // _JOB_H_
