#include "observable.hpp"

namespace ProcessingUnit
{
uint32_t Observable::subscribe(std::function<void(ObserverDataMessage &)> callback)
{
    std::lock_guard<std::mutex> lck_input_publisher(_mutex_publisher);
    _callbacks_observers_map[_counter] = callback;
    return _counter++;
}

void Observable::unsubscribe(uint32_t callback_identifier)
{
    std::lock_guard<std::mutex> lck_input_publisher(_mutex_publisher);
    auto it = _callbacks_observers_map.find(callback_identifier);
    if (it != _callbacks_observers_map.end())
    {
        _callbacks_observers_map.erase(it);
    }
}

void Observable::notify(ObserverDataMessage &input_data_message)
{
    std::lock_guard<std::mutex> lck_input_publisher(_mutex_publisher);
    for (auto it = _callbacks_observers_map.begin(); it != _callbacks_observers_map.end(); ++it)
    {
        auto callback = it->second;
        callback(input_data_message);
    }
}

} // namespace ProcessingUnit
