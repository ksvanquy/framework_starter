#include "runtime_bridge.h"

#include "example_module/example_module.h"
#include "runtime/module_manager.h"
#include "runtime/plugin_loader.h"

#include <QCoreApplication>
#include <QDir>

#include <filesystem>
#include <utility>

namespace framework::ui {

RuntimeBridge::RuntimeBridge(QObject* parent)
        : QObject(parent),
            runtime_(std::make_unique<runtime::Runtime>()) {}

RuntimeBridge::~RuntimeBridge() {
        runtime_->stop();
}

QString RuntimeBridge::state() const {
    return state_;
}

QString RuntimeBridge::lastError() const {
    return lastError_;
}

bool RuntimeBridge::moduleRegistered() const {
    return moduleRegistered_;
}

QString RuntimeBridge::exampleModuleState() const {
    switch (runtime_->moduleManager().getModuleState("example")) {
    case runtime::ModuleState::Discovered: return QStringLiteral("Discovered");
    case runtime::ModuleState::Loaded: return QStringLiteral("Loaded");
    case runtime::ModuleState::Initialized: return QStringLiteral("Initialized");
    case runtime::ModuleState::Started: return QStringLiteral("Started");
    case runtime::ModuleState::Running: return QStringLiteral("Running");
    case runtime::ModuleState::Stopping: return QStringLiteral("Stopping");
    case runtime::ModuleState::Stopped: return QStringLiteral("Stopped");
    case runtime::ModuleState::Unloaded: return QStringLiteral("Unloaded");
    }
    return QStringLiteral("Unknown");
}

bool RuntimeBridge::pluginLoaded() const {
    return pluginLoaded_;
}

void RuntimeBridge::start() {
    if (!moduleRegistered_) {
        auto result = runtime_->moduleManager().registerModule(
            std::make_unique<modules::ExampleModule>(runtime_->logger(), runtime_->eventBus()));
        if (!result) {
            setError(QString::fromStdString(result.error().message()));
            return;
        }
        moduleRegistered_ = true;
        emit moduleRegisteredChanged();
        emit exampleModuleStateChanged();
    }

    if (!pluginLoaded_) {
        const QDir pluginDirectory(QCoreApplication::applicationDirPath() + "/plugins");
        const QFileInfoList candidates = pluginDirectory.entryInfoList(
            {QStringLiteral("framework_example_plugin.*")}, QDir::Files, QDir::Name);
        if (candidates.isEmpty()) {
            setError(QStringLiteral("Example plugin was not found in %1")
                         .arg(pluginDirectory.absolutePath()));
            return;
        }

#if defined(Q_OS_WIN)
        const std::filesystem::path pluginPath(candidates.first().absoluteFilePath().toStdWString());
#else
        const std::filesystem::path pluginPath(candidates.first().absoluteFilePath().toStdString());
#endif
        auto pluginResult = runtime::PluginLoader{}.load(pluginPath);
        if (!pluginResult) {
            setError(QString::fromStdString(pluginResult.error().message()));
            return;
        }
        auto registerResult = runtime_->moduleManager().registerPlugin(std::move(pluginResult.value()));
        if (!registerResult) {
            setError(QString::fromStdString(registerResult.error().message()));
            return;
        }
        pluginLoaded_ = true;
        emit pluginLoadedChanged();
    }

    auto result = runtime_->initialize();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    result = runtime_->start();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    clearError();
    setState(QStringLiteral("Running"));
    emit exampleModuleStateChanged();
}

void RuntimeBridge::stop() {
    const auto result = runtime_->stop();
    if (pluginLoaded_) {
        pluginLoaded_ = false;
        emit pluginLoadedChanged();
    }
    emit exampleModuleStateChanged();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    clearError();
    setState(QStringLiteral("Stopped"));
}

void RuntimeBridge::reset() {
    runtime_->stop();
    runtime_ = std::make_unique<runtime::Runtime>();
    moduleRegistered_ = false;
    pluginLoaded_ = false;
    clearError();
    setState(QStringLiteral("Stopped"));
    emit moduleRegisteredChanged();
    emit exampleModuleStateChanged();
    emit pluginLoadedChanged();
}

void RuntimeBridge::clearError() {
    setError({});
}

void RuntimeBridge::setError(QString message) {
    if (lastError_ == message) {
        return;
    }
    lastError_ = std::move(message);
    emit lastErrorChanged();
    if (!lastError_.isEmpty()) {
        emit errorOccurred(lastError_);
    }
}

void RuntimeBridge::setState(QString state) {
    if (state_ == state) {
        return;
    }
    state_ = std::move(state);
    emit stateChanged();
}

} // namespace framework::ui
