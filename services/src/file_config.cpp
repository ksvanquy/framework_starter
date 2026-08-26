#include "services/file_config.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
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

bool splitLine(const std::string& line, char& type, std::string& encodedKey,
               std::string& encodedValue) {
    const auto firstSeparator = line.find('\t');
    if (firstSeparator == std::string::npos || firstSeparator == 0) {
        return false;
    }
    const auto secondSeparator = line.find('\t', firstSeparator + 1);
    if (secondSeparator == std::string::npos) {
        return false;
    }

    type = line[0];
    encodedKey = line.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
    encodedValue = line.substr(secondSeparator + 1);
    return true;
}

} // namespace

FileConfig::FileConfig(std::filesystem::path path) : path_(std::move(path)) {}

template <typename Value>
core::Result<Value> FileConfig::getValue(const std::string& key) const {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Configuration key is empty");
    }

    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    if (found == values_.end()) {
        return makeError(core::ErrorCode::NotFound, "Configuration key not found: " + key);
    }
    const auto value = std::get_if<Value>(&found->second);
    if (value == nullptr) {
        return makeError(core::ErrorCode::InvalidArgument, "Configuration value has an unexpected type: " + key);
    }
    return *value;
}

core::Result<std::string> FileConfig::getString(const std::string& key) const {
    return getValue<std::string>(key);
}

core::Result<int64_t> FileConfig::getInt(const std::string& key) const {
    return getValue<int64_t>(key);
}

core::Result<bool> FileConfig::getBool(const std::string& key) const {
    return getValue<bool>(key);
}

core::Result<double> FileConfig::getDouble(const std::string& key) const {
    return getValue<double>(key);
}

core::Result<void> FileConfig::setString(std::string key, std::string value) {
    return setValue(std::move(key), std::move(value));
}

core::Result<void> FileConfig::setInt(std::string key, int64_t value) {
    return setValue(std::move(key), value);
}

core::Result<void> FileConfig::setBool(std::string key, bool value) {
    return setValue(std::move(key), value);
}

core::Result<void> FileConfig::setDouble(std::string key, double value) {
    return setValue(std::move(key), value);
}

core::Result<void> FileConfig::setValue(std::string key, ConfigValue value) {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Configuration key is empty");
    }

    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    const auto previous = found == values_.end() ? std::optional<ConfigValue>{}
                                                  : std::optional<ConfigValue>{found->second};
    values_[key] = std::move(value);

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

core::Result<void> FileConfig::load() {
    std::ifstream input(path_);
    if (!input) {
        if (std::filesystem::exists(path_)) {
            return makeError(core::ErrorCode::InternalError,
                             "Unable to open configuration file: " + path_.string());
        }
        return {};
    }

    std::string header;
    if (!std::getline(input, header) || header != "FRAMEWORK_CONFIG_V1") {
        return makeError(core::ErrorCode::InvalidArgument,
                         "Invalid configuration file header: " + path_.string());
    }

    std::unordered_map<std::string, ConfigValue> loadedValues;
    std::string line;
    while (std::getline(input, line)) {
        char type = '\0';
        std::string encodedKey;
        std::string encodedValue;
        std::string key;
        if (!splitLine(line, type, encodedKey, encodedValue) ||
            !decodeHex(encodedKey, key) || key.empty()) {
            return makeError(core::ErrorCode::InvalidArgument,
                             "Invalid configuration entry: " + path_.string());
        }

        try {
            if (type == 's') {
                std::string value;
                if (!decodeHex(encodedValue, value)) {
                    throw std::invalid_argument("invalid string encoding");
                }
                loadedValues[key] = std::move(value);
            } else if (type == 'i') {
                loadedValues[key] = std::stoll(encodedValue);
            } else if (type == 'b' && (encodedValue == "0" || encodedValue == "1")) {
                loadedValues[key] = encodedValue == "1";
            } else if (type == 'd') {
                loadedValues[key] = std::stod(encodedValue);
            } else {
                throw std::invalid_argument("invalid configuration type");
            }
        } catch (const std::exception&) {
            return makeError(core::ErrorCode::InvalidArgument,
                             "Invalid configuration value: " + key);
        }
    }
    if (input.bad()) {
        return makeError(core::ErrorCode::InternalError,
                         "Unable to read configuration file: " + path_.string());
    }

    std::lock_guard lock(mutex_);
    values_ = std::move(loadedValues);
    return {};
}

core::Result<void> FileConfig::persistLocked() const {
    const auto parent = path_.parent_path();
    std::error_code filesystemError;
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError) {
            return makeError(core::ErrorCode::InternalError,
                             "Unable to create configuration directory: " + filesystemError.message());
        }
    }

    auto temporaryPath = path_;
    temporaryPath += ".tmp";
    std::ofstream output(temporaryPath, std::ios::trunc);
    if (!output) {
        return makeError(core::ErrorCode::InternalError,
                         "Unable to write temporary configuration file: " + temporaryPath.string());
    }

    output << "FRAMEWORK_CONFIG_V1\n";
    for (const auto& [key, value] : values_) {
        std::visit([&output, &key](const auto& typedValue) {
            using Value = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<Value, std::string>) {
                output << 's' << '\t' << encodeHex(key) << '\t' << encodeHex(typedValue) << '\n';
            } else if constexpr (std::is_same_v<Value, int64_t>) {
                output << 'i' << '\t' << encodeHex(key) << '\t' << typedValue << '\n';
            } else if constexpr (std::is_same_v<Value, bool>) {
                output << 'b' << '\t' << encodeHex(key) << '\t' << (typedValue ? '1' : '0') << '\n';
            } else if constexpr (std::is_same_v<Value, double>) {
                output << 'd' << '\t' << encodeHex(key) << '\t'
                       << std::setprecision(std::numeric_limits<double>::max_digits10)
                       << typedValue << '\n';
            }
        }, value);
    }
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporaryPath, filesystemError);
        return makeError(core::ErrorCode::InternalError,
                         "Unable to flush configuration file: " + temporaryPath.string());
    }
    output.close();

    std::filesystem::remove(path_, filesystemError);
    filesystemError.clear();
    std::filesystem::rename(temporaryPath, path_, filesystemError);
    if (filesystemError) {
        std::filesystem::remove(temporaryPath, filesystemError);
        return makeError(core::ErrorCode::InternalError,
                         "Unable to replace configuration file: " + path_.string());
    }
    return {};
}

} // namespace framework::services
