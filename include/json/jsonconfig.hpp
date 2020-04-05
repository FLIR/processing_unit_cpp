#pragma once


#include "vendor/json.hpp"

// Normally it is a big no-no to pollute the global namespace in a header file, but we use this json
// type everywhere as-if it were a standard type, and nlohman::json is just too verbose. Also, this
// is the only entry point for getting the json library included (the actual library is hidden in
// vendor/json.hpp), so there should be no surprises.
using json = nlohmann::json;

// Fetch a value if it is present, otherwise do nothing.
//
// Useful for providing a mechanism to override default parameter values.
// e.g.,
//   int myParam = 123;                     // hardcoded default value
//   json config = loadJson("config.json"); // load user configuration
//   fetch(config, "myParam", myParam);     // override default value with config["myParam"] if it exists

namespace ProcessingUnit{
template <class T>
void fetch(const json& j, const std::string& key, T& val) {
  if (!j.is_null() && j.count(key) != 0) {
    val = j[key].get<T>();
  }
}
}


