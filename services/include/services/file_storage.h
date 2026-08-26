#pragma once

#include "istorage.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace framework::services {

class FileStorage final : public IStorage {
public:
    explicit FileStorage(std::filesystem::path path);

    core::Result<void> load();

    core::Result<void> set(const std::string& key, const std::string& value) override;
    core::Result<std::string> get(const std::string& key) override;

private:
    core::Result<void> persistLocked() const;

    std::filesystem::path path_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

} // namespace framework::services
