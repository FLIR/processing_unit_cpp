#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <ctime>
#include <chrono>
#include <map>
#include <string>
#include <typeinfo>

#include "json/jsonconfig.hpp"

using namespace std;

namespace ProcessingUnit
{
typedef std::vector<char> DataPtr;

struct ObserverDataMessage
{
    ObserverDataMessage(DataPtr payload) : message_payload(payload) {}

    ObserverDataMessage(const ObserverDataMessage &result_message) : message_payload(result_message.message_payload) {}
    ObserverDataMessage &operator=(const ObserverDataMessage &other)
    {
        this->message_payload = other.message_payload;
        return *this;
    }

    DataPtr message_payload;
};

class IObservable
{
public:
    virtual uint32_t subscribe(std::function<void(ObserverDataMessage &)>) = 0;
    virtual void unsubscribe(uint32_t) = 0;
    virtual void notify(ObserverDataMessage &) = 0;
};

class Observable : public IObservable
{
public:
    Observable(){};
    uint32_t subscribe(std::function<void(ObserverDataMessage &)>) override;
    void unsubscribe(uint32_t callback_identifier) override;
    void notify(ObserverDataMessage &) override;

private:
    std::mutex _mutex_publisher;
    std::map<uint32_t, std::function<void(ObserverDataMessage &)>> _callbacks_observers_map;
    uint32_t _counter = 0;
};
} // namespace ProcessingUnit
