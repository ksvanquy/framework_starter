#pragma once
#include <cstdint>
#include <string>
#include "core/result.h"

namespace framework::services {
class IConfig {
public:
    virtual ~IConfig() = default;

    virtual core::Result<std::string> getString(const std::string& key) const = 0;
    virtual core::Result<int64_t> getInt(const std::string& key) const = 0;
    virtual core::Result<bool> getBool(const std::string& key) const = 0;
    virtual core::Result<double> getDouble(const std::string& key) const = 0;

    virtual core::Result<void> setString(std::string key, std::string value) = 0;
    virtual core::Result<void> setInt(std::string key, int64_t value) = 0;
    virtual core::Result<void> setBool(std::string key, bool value) = 0;
    virtual core::Result<void> setDouble(std::string key, double value) = 0;
};
} // namespace framework::services