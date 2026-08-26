#pragma once

#include "services/icommand_bus.h"
#include "services/iconfig.h"
#include "services/idiagnostics.h"
#include "services/ievent_bus.h"
#include "services/ilogger.h"
#include "services/ischeduler.h"
#include "services/istorage.h"

namespace framework::runtime {

// RuntimeContext is a non-owning dependency bundle supplied by the composition root.
// The composition root owns the referenced services and must destroy them only after
// Runtime has stopped and unloaded all modules and plugins using this context.
struct RuntimeContext {
    services::ILogger& logger;
    services::IEventBus& eventBus;
    services::IConfig& config;
    services::ICommandBus& commandBus;
    services::IScheduler& scheduler;
    services::IStorage& storage;
    services::IDiagnostics& diagnostics;
};

} // namespace framework::runtime