#pragma once

#include <memory>

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "runtime/runtime.h"
#include "services/default_services.h"
#include "services/file_config.h"
#include "services/file_storage.h"

namespace framework::ui {

class RuntimeBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool moduleRegistered READ moduleRegistered NOTIFY moduleRegisteredChanged)
    Q_PROPERTY(QString exampleModuleState READ exampleModuleState NOTIFY exampleModuleStateChanged)
    Q_PROPERTY(bool pluginLoaded READ pluginLoaded NOTIFY pluginLoadedChanged)

public:
    explicit RuntimeBridge(QObject* parent = nullptr);
    ~RuntimeBridge() override;

    [[nodiscard]] QString state() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool moduleRegistered() const;
    [[nodiscard]] QString exampleModuleState() const;
    [[nodiscard]] bool pluginLoaded() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void clearError();

signals:
    void stateChanged();
    void moduleRegisteredChanged();
    void exampleModuleStateChanged();
    void pluginLoadedChanged();
    void lastErrorChanged();
    void errorOccurred(const QString& message);

private:
    void setError(QString message);
    void setState(QString state);

    services::ConsoleLogger logger_;
    services::FileConfig config_;
    services::InMemoryEventBus eventBus_;
    services::InMemoryCommandBus commandBus_;
    services::ThreadScheduler scheduler_;
    services::FileStorage storage_;
    services::BasicDiagnostics diagnostics_;
    std::unique_ptr<runtime::Runtime> runtime_;
    bool configReady_ = false;
    bool moduleRegistered_ = false;
    bool pluginLoaded_ = false;
    QString state_ = QStringLiteral("Stopped");
    QString lastError_;
};

} // namespace framework::ui
