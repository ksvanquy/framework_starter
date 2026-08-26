TASK: TẠO APPLICATION MỚI "Framework IoT Control Center"

Repository:
https://github.com/ksvanquy/framework_starter

============================================================
MỤC TIÊU
============================================================

Tạo một APPLICATION MỚI độc lập:

    apps/iot_control_center/

Không thay thế và không biến apps/qt6_app thành IoT application.

apps/qt6_app phải được giữ nguyên như framework Qt6 example hiện tại.

Framework IoT Control Center là một Qt6/QML desktop application THỰC TẾ
dùng framework_starter làm application framework.

Mục tiêu của application này là trở thành REFERENCE APPLICATION:

    framework_starter
          ↓
    real application
          ↓
    IoT Control Center

Application phải chứng minh framework có thể được sử dụng để xây dựng
một application có domain, business logic, modules, plugins, services,
configuration, persistence, events, commands, scheduling, diagnostics
và error handling.

Đây KHÔNG phải framework test dashboard.

Không tạo các màn hình kiểu:

    "EventBus: PASS"
    "Scheduler: PASS"
    "Storage: PASS"

thay vào đó, các capability của framework phải được sử dụng bởi
nghiệp vụ IoT thực tế.

============================================================
1. GIỮ NGUYÊN qt6_app
============================================================

KHÔNG:

- rename qt6_app
- xóa qt6_app
- nhồi IoT logic vào qt6_app
- thay đổi behavior hiện tại của qt6_app nếu không cần thiết

qt6_app vẫn là:

    framework Qt6/QML example
    runtime/plugin loading example

Application mới phải là:

    apps/iot_control_center/

============================================================
2. DOMAIN
============================================================

Xây dựng ứng dụng:

    Framework IoT Control Center

Ứng dụng quản lý một Smart Home / IoT Lab giả lập.

Không cần hardware thật.
Không cần network thật.
Không cần MQTT thật.

Mọi thiết bị phải deterministic và chạy local.

Thiết bị:

    TemperatureSensor
    HumiditySensor
    LightController
    AirConditioner
    DoorSensor

Tạo một DemoDevicePlugin để cung cấp các thiết bị giả lập.

============================================================
3. USER EXPERIENCE
============================================================

Ứng dụng phải trông và hoạt động như một application thực tế.

Main layout:

    ┌──────────────────────────────────────────────────────────┐
    │ Framework IoT Control Center              ● All Systems OK│
    ├──────────────┬───────────────────────────────────────────┤
    │ Dashboard    │                                           │
    │ Devices      │                                           │
    │ Automations  │                                           │
    │ Events       │                                           │
    │ Diagnostics  │                                           │
    │ Settings     │                                           │
    │              │                                           │
    │              │                                           │
    └──────────────┴───────────────────────────────────────────┘

Không tạo UI chỉ để test framework.

UI phải tập trung vào:

    devices
    rooms
    sensor values
    device state
    automations
    events
    system health

============================================================
4. DEVICE DASHBOARD
============================================================

Dashboard hiển thị:

    Total devices
    Online devices
    Offline devices
    Active automations
    Recent events
    Current temperature
    Current humidity

Ví dụ:

    Living Room
    ─────────────────────────────

    Temperature       24.8 °C
    Humidity          62 %

    Air Conditioner   OFF
    Light             ON

    Door              CLOSED


    Bedroom
    ─────────────────────────────

    Temperature       27.1 °C
    Humidity          58 %

    Air Conditioner   ON
    Door              CLOSED

Values phải đến từ application/domain thực tế.

Không hard-code UI values.

============================================================
5. DEVICE MANAGEMENT
============================================================

Devices page:

    Device list

    Name
    Type
    Room
    Status
    Last update

Ví dụ:

    Living Room Temperature Sensor
    Living Room Humidity Sensor
    Living Room Light
    Bedroom Air Conditioner
    Front Door Sensor

Device detail:

    Device information
    Current state
    Last update
    Controls
    Recent history

============================================================
6. DEVICE CONTROL
============================================================

Các device có khả năng phù hợp.

LightController:

    ON
    OFF
    brightness

AirConditioner:

    ON
    OFF
    target temperature

DoorSensor:

    OPEN
    CLOSED

Sensor:

    current value

Tất cả device actions phải đi qua:

    ICommandBus

Không cho QML gọi trực tiếp implementation của device.

Flow:

    QML
      ↓
    Qt adapter
      ↓
    Application service
      ↓
    ICommandBus
      ↓
    DeviceManager
      ↓
    Device plugin
      ↓
    Device

============================================================
7. DEVICE EVENTS
============================================================

Sử dụng IEventBus.

Các domain events:

    DeviceConnected
    DeviceDisconnected
    DeviceStateChanged
    SensorValueChanged
    CommandExecuted
    AutomationTriggered

Ví dụ:

    TemperatureSensor
          ↓
    SensorValueChanged
          ↓
    EventBus
          ↓
    Monitoring
          ↓
    Automation
          ↓
    UI

Không dùng signal/slot trực tiếp để thay thế EventBus cho
application/domain events.

Qt signals chỉ dùng cho presentation/QML notification khi phù hợp.

============================================================
8. AUTOMATION
============================================================

Tạo automation engine thực tế.

Ví dụ rule:

    IF temperature > 28°C
    THEN turn ON air conditioner

Một rule khác:

    IF door opens
    THEN turn ON living room light

UI:

    Automations

    ☑ Cool bedroom

    IF
        Bedroom temperature > 28 °C

    THEN
        Bedroom AC → ON

    Status: Active
    Executions: 12
    Last execution: 10:42:15

Automation rules phải được lưu.

============================================================
9. SCHEDULER
============================================================

Sử dụng IScheduler.

Scheduler dùng cho:

    sensor polling
    automation evaluation
    health monitoring
    periodic persistence nếu phù hợp

Ví dụ:

    Sensor update       every 1 second
    Automation check   every 1 second
    Health check       every 5 seconds

Không dùng QTimer trong business/application layer để thay thế
IScheduler.

QML/UI timers không được dùng để giả lập framework scheduler.

============================================================
10. SENSOR SIMULATION
============================================================

DemoDevicePlugin phải mô phỏng sensor values.

Temperature:

    dao động nhẹ theo thời gian

Humidity:

    dao động nhẹ

Door:

    có thể thay đổi bằng UI

Light:

    ON/OFF/brightness

Air conditioner:

    ON/OFF
    target temperature

Simulation phải deterministic đủ để integration test.

Có thể thêm Developer/Simulation controls:

    Increase temperature
    Decrease temperature
    Open door
    Close door
    Disconnect device

============================================================
11. PLUGIN ARCHITECTURE
============================================================

DemoDevicePlugin phải sử dụng plugin contract THỰC TẾ của framework.

Không tạo plugin system riêng.

Sử dụng:

    framework::runtime::PluginLoader

và public plugin API hiện có.

Plugin phải expose các device modules/drivers thông qua
framework plugin mechanism.

Application composition root phải load plugin giống cách framework
được thiết kế.

Không hard-code device implementation trực tiếp vào Runtime.

============================================================
12. APPLICATION MODULES
============================================================

Tạo các application-owned modules:

    DeviceManagerModule
    AutomationModule
    MonitoringModule

Dependencies:

    DeviceManagerModule
            ↑
            │
    ┌───────┴────────┐
    │                │
AutomationModule MonitoringModule

Module lifecycle phải được quản lý bởi framework Runtime/ModuleManager.

Không tự tạo module manager thứ hai.

============================================================
13. LOGGER
============================================================

Sử dụng ILogger thực tế.

Log:

    device connected
    device disconnected
    command execution
    automation trigger
    plugin lifecycle
    errors
    warnings

Không dùng qDebug() làm logging mechanism của application.

Có Logs page để người dùng xem log gần đây.

============================================================
14. CONFIGURATION
============================================================

Sử dụng IConfig.

Lưu:

    application settings
    device settings
    automation settings

Ví dụ:

    application.theme
    application.poll_interval

    devices.bedroom_ac.target_temperature

    automation.cool_bedroom.enabled
    automation.cool_bedroom.threshold

Không truy cập config implementation trực tiếp nếu framework cung cấp
IConfig abstraction.

============================================================
15. STORAGE
============================================================

Sử dụng IStorage.

Lưu:

    device state
    sensor history
    event history
    automation rules

Khi application restart:

    Runtime start
        ↓
    Storage restore
        ↓
    devices restored
        ↓
    automation rules restored
        ↓
    application continues

Không dùng file I/O trực tiếp trong application để bypass IStorage.

============================================================
16. DIAGNOSTICS
============================================================

Sử dụng IDiagnostics.

Diagnostics phải phục vụ vận hành application.

Hiển thị:

    Runtime status
    Module status
    Plugin status
    Device count
    Online/offline count
    Scheduler status
    Recent errors
    Recent warnings

Đây là operational diagnostics,
KHÔNG phải một "framework feature checklist".

============================================================
17. ERROR HANDLING
============================================================

Dùng Result<T> và error contract thực tế của framework.

Không crash khi:

    device disconnect
    command fails
    plugin fails
    dependency missing
    invalid configuration
    storage error

Ví dụ:

    User clicks "Turn ON AC"

    CommandBus
        ↓
    DeviceManager
        ↓
    Device plugin
        ↓
    Result<void>

Nếu fail:

    UI hiển thị:

    "Unable to turn on Bedroom AC"

    + error information

Không catch rồi bỏ qua lỗi.

Không return fake success.

============================================================
18. FAULT SIMULATION
============================================================

Tạo:

    Developer → Simulation

Nhưng đây vẫn là application functionality,
không phải framework dashboard.

Cho phép:

    Disconnect selected device
    Reconnect device
    Simulate command failure
    Simulate plugin failure
    Simulate storage failure
    Simulate invalid configuration
    Force dependency failure

Mục đích:

    chứng minh application gracefully handles real-world failures.

============================================================
19. QML ARCHITECTURE
============================================================

Architecture:

    QML
      ↓
    Qt/QML Adapter
      ↓
    Application Service
      ↓
    Framework Runtime/Services
      ↓
    Modules
      ↓
    Plugins

QML:

    presentation
    user interaction
    models/view models

QML KHÔNG được chứa:

    framework logic
    plugin loading
    device business logic
    persistence
    automation engine

============================================================
20. APPLICATION STRUCTURE
============================================================

Tạo:

    apps/iot_control_center/

Ví dụ:

    apps/iot_control_center/
    ├── CMakeLists.txt
    ├── README.md
    ├── src/
    │   ├── application/
    │   ├── devices/
    │   ├── automation/
    │   ├── monitoring/
    │   ├── adapters/
    │   └── main.cpp
    │
    ├── qml/
    │   ├── Main.qml
    │   ├── pages/
    │   │   ├── DashboardPage.qml
    │   │   ├── DevicesPage.qml
    │   │   ├── DeviceDetailPage.qml
    │   │   ├── AutomationsPage.qml
    │   │   ├── EventsPage.qml
    │   │   ├── LogsPage.qml
    │   │   ├── DiagnosticsPage.qml
    │   │   └── SettingsPage.qml
    │   └── components/
    │
    ├── plugins/
    │   └── demo_device_plugin/
    │
    └── tests/
        ├── integration/
        └── application/

Cấu trúc thực tế có thể điều chỉnh sau khi audit repository.

Không tạo abstraction chỉ vì đẹp kiến trúc.

============================================================
21. CMAKE
============================================================

Thêm option riêng:

    BUILD_IOT_CONTROL_CENTER

Ví dụ:

    cmake -S . -B build-iot \
        -DBUILD_PLUGINS=ON \
        -DBUILD_IOT_CONTROL_CENTER=ON

Build target:

    framework_iot_control_center

Không làm hỏng:

    framework_qt6_app

Plugin DemoDevicePlugin phải được build/copy theo cơ chế phù hợp
với framework hiện tại.

Không tạo build system song song.

============================================================
22. TESTS
============================================================

Viết tests cho application behavior.

Bắt buộc:

    Runtime startup
    Module dependency
    Plugin loading
    Device discovery
    Device command
    Event propagation
    Scheduler
    Config persistence
    Storage persistence
    Automation
    Diagnostics
    Plugin unload
    Runtime shutdown
    Failure recovery

Quan trọng:

Tests phải test DOMAIN BEHAVIOR.

Ví dụ:

    Given temperature > 28°C
    When automation evaluation runs
    Then AC receives TurnOn command

Không chỉ:

    ASSERT_TRUE(eventBus.exists())

============================================================
23. END-TO-END TEST
============================================================

Phải có ít nhất một full integration scenario:

    Start application
        ↓
    Runtime initialize
        ↓
    DeviceManager start
        ↓
    Automation start
        ↓
    Monitoring start
        ↓
    DemoDevicePlugin load
        ↓
    Devices discovered
        ↓
    Sensor polling
        ↓
    SensorValueChanged
        ↓
    Automation evaluates condition
        ↓
    CommandBus
        ↓
    TurnOn Air Conditioner
        ↓
    DeviceStateChanged
        ↓
    EventBus
        ↓
    Storage persists state
        ↓
    Diagnostics reflects state
        ↓
    Shutdown
        ↓
    Plugin unload
        ↓
    Modules stop
        ↓
    Runtime stop

Mỗi bước phải kiểm tra Result/error.

============================================================
24. RESOURCE CLEANUP
============================================================

Đảm bảo:

    plugin unload
    module stop
    scheduler cancellation
    event subscription cleanup
    QObject lifetime
    RuntimeBridge lifetime
    plugin library handle cleanup

Không có:

    dangling pointers
    duplicate subscriptions
    timer leaks
    plugin handle leaks

Nếu repository hỗ trợ:

    ASan
    UBSan

hãy chạy integration tests với sanitizer.

============================================================
25. DOCUMENTATION
============================================================

Tạo:

    docs/qt6_iot_reference_app.md

Document phải mô tả:

    Application architecture
    Domain model
    Module architecture
    Plugin architecture
    Event flow
    Command flow
    Persistence
    Automation
    Error handling
    Build
    Run
    Test

Quan trọng nhất:

Framework capability
        ↓
IoT application feature
        ↓
Source code
        ↓
Integration test

Ví dụ:

    IEventBus
        ↓
    DeviceConnected / SensorValueChanged
        ↓
    DeviceManager + Monitoring
        ↓
    event integration tests

Không tạo bảng PASS giả.

============================================================
26. FRAMEWORK AUDIT
============================================================

TRƯỚC KHI CODE:

Đọc:

    README.md
    ARCHITECTURE.md
    core/
    services/
    runtime/
    modules/
    plugins/
    apps/qt6_app/
    tests nếu có
    docs/

Xác định API THỰC TẾ:

    Runtime
    ModuleManager
    PluginLoader
    IModule
    ILogger
    IConfig
    IEventBus
    ICommandBus
    IScheduler
    IStorage
    IDiagnostics
    Result<T>
    Plugin ABI

Không đoán API.

Không tạo wrapper giả để bù cho việc chưa đọc framework.

============================================================
27. QUAN TRỌNG: KHÔNG BẮT FRAMEWORK PHẢI CÓ
============================================================

Nếu một capability được yêu cầu ở trên nhưng framework hiện tại
KHÔNG có API đủ để hỗ trợ nó:

KHÔNG:

    fake implementation
    bypass abstraction
    hard-code behavior
    tạo parallel framework service

Thay vào đó:

1. Xác định limitation.
2. Xác định API framework cần thiết.
3. Đánh giá đó có phải thiếu sót thực sự của framework không.
4. Nếu hợp lý, bổ sung capability vào framework theo architecture hiện tại.
5. Viết framework tests.
6. Sau đó integrate vào IoT application.

Mọi framework change phải giữ generic.

Không đưa IoT-specific code vào framework.

Ví dụ SAI:

    framework::TemperatureSensor

Ví dụ ĐÚNG:

    generic framework service/API

IoT domain chỉ nằm trong:

    apps/iot_control_center/

============================================================
28. DEFINITION OF DONE
============================================================

DONE khi repository có:

    apps/qt6_app/
        unchanged as existing Qt example

    apps/iot_control_center/
        complete real application

Application phải:

    build
    run
    load DemoDevicePlugin
    discover devices
    display devices
    control devices
    generate events
    execute commands
    run scheduler
    run automation
    persist configuration
    persist state/history
    expose diagnostics
    handle errors
    recover from simulated failures
    shutdown cleanly

Tests phải PASS.

============================================================
29. FINAL REPORT
============================================================

Sau khi implementation hoàn thành, báo cáo:

## Files added

...

## Framework changes

...

## IoT application architecture

...

## Framework capabilities actually used

    Runtime        → application lifecycle
    ModuleManager  → Device/Automation/Monitoring
    PluginLoader   → DemoDevicePlugin
    ILogger        → application logging
    IConfig        → settings
    IEventBus      → domain events
    ICommandBus    → device commands
    IScheduler     → polling/automation
    IStorage       → persistence
    IDiagnostics   → operational health
    Result<T>      → error handling

## Tests

    Build
    Unit
    Integration
    End-to-end
    Sanitizer

## Known limitations

Liệt kê trung thực.

Đặc biệt:

Nếu framework có capability chưa thể dùng đúng cách,
KHÔNG đánh dấu hoàn thành.

============================================================
30. IMPLEMENTATION ORDER
============================================================

PHASE 1:
Audit repository.

PHASE 2:
Thiết kế IoT application architecture dựa trên API thực tế.

PHASE 3:
Implement domain model.

PHASE 4:
Implement DeviceManagerModule.

PHASE 5:
Implement DemoDevicePlugin.

PHASE 6:
Implement MonitoringModule.

PHASE 7:
Implement AutomationModule.

PHASE 8:
Integrate framework services.

PHASE 9:
Implement Qt/QML adapters.

PHASE 10:
Implement UI.

PHASE 11:
Implement persistence.

PHASE 12:
Implement fault simulation.

PHASE 13:
Implement integration tests.

PHASE 14:
Build/run/test.

PHASE 15:
Fix all issues.

PHASE 16:
Create docs/qt6_iot_reference_app.md.

BẮT ĐẦU NGAY BẰNG AUDIT.
KHÔNG CODE TRƯỚC KHI HIỂU PUBLIC API CỦA FRAMEWORK.