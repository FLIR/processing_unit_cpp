
#include <thread>
#include <chrono>

#include "spdlog/spdlog.h" // logging
#include "job.hpp"
#include "job_connection.hpp"

namespace ProcessingUnit
{
const std::string NameLogger("MainLogger");

Job::Job(const json &config, JobConnection *job_con) : m_JobConnection(job_con)
{
	spdlog::get(NameLogger)->trace(config.dump(4));
	std::string jsonString(config.dump());
	m_ProcessThread = std::thread(&Job::Processing, this);
}

Job::~Job()
{
	spdlog::get(NameLogger)->trace("[Job::process]: Job just destructed");
	m_JobConnection->UnsubscribeProcessorResult(_callback_identifier);
}

json Job::process(DataPtr &data)
{
	spdlog::get(NameLogger)->trace("[Job::process]: adding data to process");
	m_DataProtector.lock();
	m_InputData.push(data);
	m_isJobEmpty = false;
	m_DataProtector.unlock();
	m_ConditionVariable.notify_one();
	return json{{"OK", "Echoing"}};
}

bool Job::readData(DataPtr *data)
{
	if (!data)
	{
		return false;
	}

	std::unique_lock<std::mutex> data_protector_mutex(m_DataProtector);
	if (m_InputData.size())
	{
		spdlog::get(NameLogger)->trace("[Job::process]: processing read");
		*data = m_InputData.front();
		m_InputData.pop();
		if (m_InputData.size() == 0)
		{
			m_isJobEmpty = true;
		}
		data_protector_mutex.unlock();
		m_JobConnection->SendContinue();
		return true;
	}

	return false;
}

void Job::writeData(DataPtr &data)
{
	spdlog::get(NameLogger)->trace("[Job::process]: processing write result");
	m_JobConnection->SendData(data);
}

void Job::stopJob()
{
	spdlog::get(NameLogger)->trace("[Job::process]: stopping Job");
	std::unique_lock<std::mutex> data_protector_mutex(m_DataProtector);
	m_isStopJobSignaled = true;
	data_protector_mutex.unlock();
	m_ConditionVariable.notify_one();
	m_ProcessThread.join();
}

bool Job::isStoped()
{
	std::lock_guard<std::mutex> data_protector_mutex(m_DataProtector);
	return m_isStopJobSignaled & m_isJobEmpty;
}

void Job::Processing()
{
	spdlog::get(NameLogger)->trace("[Job::process]: Thread started");

	auto processor_result_callback = [&](ObserverDataMessage &result_data_message) {
		try
		{
			std::unique_lock<std::mutex> lck(m_DataProtector);

			if (result_data_message.message_payload != DataPtr())
			{
				writeData(result_data_message.message_payload);
			}
			m_isVARecived = true;
			lck.unlock();
			m_ConVarVARecived.notify_one();
		}
		catch (const std::exception &e)
		{
			std::cerr << "[Job::process]: Error: " << e.what() << std::endl;
		}
	};

	_callback_identifier = m_JobConnection->SubscribeProcessorResult(processor_result_callback);

	while (!m_isStopJobSignaled || !m_isJobEmpty)
	{
		std::unique_lock<std::mutex> data_protector_lck(m_DataProtector);
		m_ConditionVariable.wait(data_protector_lck, [&] { return m_isStopJobSignaled || !m_isJobEmpty; });
		data_protector_lck.unlock();
		DataPtr data;
		try
		{
			if (readData(&data))
			{
				ObserverDataMessage input_data_message = ObserverDataMessage(data);
				m_JobConnection->NotifyInputData(input_data_message);
				std::unique_lock<std::mutex> lck(m_DataProtector);
				m_ConVarVARecived.wait(lck, [&] { return m_isVARecived; });
				m_isVARecived = false;
			}
		}
		catch (const std::exception &e)
		{
			std::cerr << "[Job::process]: Error: " << e.what() << std::endl;
		}
	}

	DataPtr data;
	ObserverDataMessage input_data_message = ObserverDataMessage(data);
	m_JobConnection->NotifyInputData(input_data_message);
	spdlog::get(NameLogger)->trace("[Job::process]: Ending processing thread");
}
} // namespace ProcessingUnit