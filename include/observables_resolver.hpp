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
class ObservablesResolver
{
public:
    static std::shared_ptr<IObservable> getInputObservable()
    {
        auto instance = getInstance();
        return instance->_input_observable;
    }

    static std::shared_ptr<IObservable> getProcessorResultObservable()
    {
        auto instance = getInstance();
        return instance->_processor_result_observable;
    }

private:
    ObservablesResolver()
    {
        _input_observable = std::shared_ptr<IObservable>(new Observable);
        _processor_result_observable = std::shared_ptr<IObservable>(new Observable);
    }

    static std::shared_ptr<ObservablesResolver> &getInstance()
    {
        static std::shared_ptr<ObservablesResolver> instance(new ObservablesResolver);
        return instance;
    }

    std::shared_ptr<IObservable> _input_observable;
    std::shared_ptr<IObservable> _processor_result_observable;
};
} // namespace ProcessingUnit
