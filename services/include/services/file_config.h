#pragma once

#include "iconfig.h"

#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <variant>

namespace framework::services {

class FileConfig final : public IConfig {
public:
    explicit FileConfig(std::filesystem::path path);

    core::Result<void> load();

    core::Result<std::string> getString(const std::string& key) const override;
    core::Result<int64_t> getInt(const std::string& key) const override;
    core::Result<bool> getBool(const std::string& key) const override;
    core::Result<double> getDouble(const std::string& key) const override;

    core::Result<void> setString(std::string key, std::string value) override;
    core::Result<void> setInt(std::string key, int64_t value) override;
    core::Result<void> setBool(std::string key, bool value) override;
    core::Result<void> setDouble(std::string key, double value) override;

private:
    using ConfigValue = std::variant<std::string, int64_t, bool, double>;

    template <typename Value>
    core::Result<Value> getValue(const std::string& key) const;
    core::Result<void> setValue(std::string key, ConfigValue value);
    core::Result<void> persistLocked() const;

    std::filesystem::path path_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ConfigValue> values_;
};

} // namespace framework::services
