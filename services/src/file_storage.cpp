#include "services/file_storage.h"

#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace framework::services {
namespace {

core::Error makeError(core::ErrorCode code, std::string message) {
    return core::Error(code, std::move(message));
}

std::string encodeHex(const std::string& value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const unsigned char character : value) {
        output << std::setw(2) << static_cast<unsigned int>(character);
    }
    return output.str();
}

bool decodeHex(const std::string& encoded, std::string& value) {
    if (encoded.size() % 2 != 0) {
        return false;
    }

    value.clear();
    value.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        unsigned int character = 0;
        std::istringstream input(encoded.substr(index, 2));
        input >> std::hex >> character;
        if (input.fail()) {
            return false;
        }
        value.push_back(static_cast<char>(character));
    }
    return true;
}

} // namespace

FileStorage::FileStorage(std::filesystem::path path) : path_(std::move(path)) {}

core::Result<void> FileStorage::load() {
    std::ifstream input(path_);
    if (!input) {
        if (std::filesystem::exists(path_)) {
            return makeError(core::ErrorCode::InternalError,
                             "Unable to open storage file: " + path_.string());
        }
        return {};
    }

    std::string header;
    if (!std::getline(input, header) || header != "FRAMEWORK_STORAGE_V1") {
        return makeError(core::ErrorCode::InvalidArgument,
                         "Invalid storage file header: " + path_.string());
    }

    std::unordered_map<std::string, std::string> loadedValues;
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('\t');
        if (separator == std::string::npos) {
            return makeError(core::ErrorCode::InvalidArgument,
                             "Invalid storage entry: " + path_.string());
        }

        std::string key;
        std::string value;
        if (!decodeHex(line.substr(0, separator), key) || key.empty() ||
            !decodeHex(line.substr(separator + 1), value)) {
            return makeError(core::ErrorCode::InvalidArgument,
                             "Invalid storage entry: " + path_.string());
        }
        loadedValues[std::move(key)] = std::move(value);
    }
    if (input.bad()) {
        return makeError(core::ErrorCode::InternalError,
                         "Unable to read storage file: " + path_.string());
    }

    std::lock_guard lock(mutex_);
    values_ = std::move(loadedValues);
    return {};
}

core::Result<void> FileStorage::set(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Storage key is empty");
    }

    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    const auto previous = found == values_.end()
        ? std::optional<std::string>{}
        : std::optional<std::string>{found->second};
    values_[key] = value;

    const auto result = persistLocked();
    if (!result) {
        if (previous.has_value()) {
            values_[key] = std::move(*previous);
        } else {
            values_.erase(key);
        }
    }
    return result;
}

core::Result<std::string> FileStorage::get(const std::string& key) {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Storage key is empty");
    }

    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    if (found == values_.end()) {
        return makeError(core::ErrorCode::NotFound, "Storage key not found: " + key);
    }
    return found->second;
}

core::Result<void> FileStorage::persistLocked() const {
    const auto parent = path_.parent_path();
    std::error_code filesystemError;
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError) {
            return makeError(core::ErrorCode::InternalError,
                             "Unable to create storage directory: " + filesystemError.message());
        }
    }

    auto temporaryPath = path_;
    temporaryPath += ".tmp";
    std::ofstream output(temporaryPath, std::ios::trunc);
    if (!output) {
        return makeError(core::ErrorCode::InternalError,
                         "Unable to write temporary storage file: " + temporaryPath.string());
    }

    output << "FRAMEWORK_STORAGE_V1\n";
    for (const auto& [key, value] : values_) {
        output << encodeHex(key) << '\t' << encodeHex(value) << '\n';
    }
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporaryPath, filesystemError);
        return makeError(core::ErrorCode::InternalError,
                         "Unable to flush storage file: " + temporaryPath.string());
    }
    output.close();

    auto backupPath = path_;
    backupPath += ".bak";
    std::filesystem::remove(backupPath, filesystemError);
    filesystemError.clear();
    const bool hadPreviousFile = std::filesystem::exists(path_, filesystemError);
    if (filesystemError) {
        std::filesystem::remove(temporaryPath, filesystemError);
        return makeError(core::ErrorCode::InternalError,
                         "Unable to inspect storage file: " + path_.string());
    }

    if (hadPreviousFile) {
        std::filesystem::rename(path_, backupPath, filesystemError);
        if (filesystemError) {
            std::filesystem::remove(temporaryPath, filesystemError);
            return makeError(core::ErrorCode::InternalError,
                             "Unable to preserve storage file: " + path_.string());
        }
    }

    filesystemError.clear();
    std::filesystem::rename(temporaryPath, path_, filesystemError);
    if (filesystemError) {
        std::filesystem::remove(temporaryPath, filesystemError);
        if (hadPreviousFile) {
            std::error_code restoreError;
            std::filesystem::rename(backupPath, path_, restoreError);
        }
        return makeError(core::ErrorCode::InternalError,
                         "Unable to replace storage file: " + path_.string());
    }

    if (hadPreviousFile) {
        std::filesystem::remove(backupPath, filesystemError);
    }
    return {};
}

} // namespace framework::services
