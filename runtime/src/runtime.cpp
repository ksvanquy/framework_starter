#include "runtime/runtime.h"

#include "runtime/module_manager.h"

#include <utility>

namespace framework::runtime {

class Runtime::Impl {
public:
    explicit Impl(RuntimeContext services)
        : context(services),
          moduleManager(context.logger) {}

    RuntimeContext context;
    ModuleManager moduleManager;
    bool initialized = false;
    bool started = false;
};

Runtime::Runtime(RuntimeContext services) : impl_(std::make_unique<Impl>(services)) {}

Runtime::~Runtime() {
    stop();
}

core::Result<void> Runtime::initialize() {
    if (impl_->initialized) {
        return {};
    }
    auto result = impl_->moduleManager.initializeAll();
    if (!result) {
        impl_->context.logger.log(services::LogLevel::Error, "Runtime", "Runtime initialization failed");
        return result;
    }
    impl_->initialized = true;
    impl_->context.logger.log(services::LogLevel::Info, "Runtime", "Runtime initialized");
    return {};
}

core::Result<void> Runtime::start() {
    if (!impl_->initialized) {
        return core::Error(core::ErrorCode::StateError, "Runtime must be initialized before start");
    }
    if (impl_->started) {
        return {};
    }
    auto result = impl_->moduleManager.startAll();
    if (!result) {
        impl_->context.logger.log(services::LogLevel::Error, "Runtime", "Runtime start failed");
        return result;
    }
    impl_->started = true;
    impl_->context.logger.log(services::LogLevel::Info, "Runtime", "Runtime started");
    return {};
}

core::Result<void> Runtime::stop() {
    if (!impl_) {
        return {};
    }
    if (!impl_->initialized) {
        return impl_->moduleManager.unloadAllPlugins();
    }
    if (!impl_->started) {
        impl_->initialized = false;
        return impl_->moduleManager.unloadAllPlugins();
    }

    auto result = impl_->moduleManager.stopAll();
    auto unloadResult = impl_->moduleManager.unloadAllPlugins();
    impl_->started = false;
    impl_->initialized = false;
    if (!result) {
        impl_->context.logger.log(services::LogLevel::Error, "Runtime", "Runtime stop failed");
        return result;
    }
    if (!unloadResult) {
        impl_->context.logger.log(services::LogLevel::Error, "Runtime", "Plugin unload failed");
        return unloadResult;
    }
    impl_->context.logger.log(services::LogLevel::Info, "Runtime", "Runtime stopped");
    return {};
}

services::ILogger& Runtime::logger() const {
    return impl_->context.logger;
}

services::IEventBus& Runtime::eventBus() const {
    return impl_->context.eventBus;
}

RuntimeContext& Runtime::context() {
    return impl_->context;
}

ModuleManager& Runtime::moduleManager() {
    return impl_->moduleManager;
}

} // namespace framework::runtime
