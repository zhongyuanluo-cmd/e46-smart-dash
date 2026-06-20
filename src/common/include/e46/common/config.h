#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <sstream>
#include <fstream>
#include "nlohmann/json.hpp"

namespace e46 {
namespace common {

class Config
{
public:
    /**
     * Load JSON config from a file. Logger should be initialized before
     * calling this method so that warning/error messages are routed correctly.
     *
     * @return true on success, false if file not found or parse error.
     */
    bool load(const std::string& path);

    template<typename T>
    T get(const std::string& key, const T& defaultVal = T{}) const;

    bool has(const std::string& key) const;

private:
    nlohmann::json data_;

    /**
     * Navigate a dot-separated path of object keys.
     * Only plain object-key navigation is supported; array indices
     * (e.g. "sensors[0].pin") are NOT supported.
     *
     * Example: "can.spi.speed" → data_["can"]["spi"]["speed"]
     *          "sensors[0].pin" → returns nullptr (unsupported)
     */
    const nlohmann::json* navigate(const std::string& dottedKey) const;
};

template<typename T>
T Config::get(const std::string& key, const T& defaultVal) const
{
    const auto* node = navigate(key);
    if (!node) return defaultVal;
    try { return node->get<T>(); }
    catch (...) { return defaultVal; }
}

} // namespace common
} // namespace e46
