#include "e46/common/config.h"
#include "e46/common/logging.h"

namespace e46 {
namespace common {

bool Config::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        E46_LOG_WARN("Config file not found: %s, using defaults", path.c_str());
        return false;
    }

    try {
        data_ = nlohmann::json::parse(file);
        E46_LOG_INFO("Config loaded from %s", path.c_str());
        return true;
    } catch (const nlohmann::json::exception& e) {
        E46_LOG_ERROR("Config parse error in %s: %s", path.c_str(), e.what());
        return false;
    }
}

bool Config::has(const std::string& key) const
{
    return navigate(key) != nullptr;
}

const nlohmann::json* Config::navigate(const std::string& dottedKey) const
{
    const nlohmann::json* current = &data_;
    std::istringstream ss(dottedKey);
    std::string segment;

    while (std::getline(ss, segment, '.')) {
        if (!current->is_object() || !current->contains(segment)) {
            return nullptr;
        }
        current = &(*current)[segment];
    }
    return current;
}

} // namespace common
} // namespace e46
