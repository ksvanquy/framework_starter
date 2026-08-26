# Blueprint Qt6/QML starter

Tài liệu này mô tả `apps/qt6_app`, ứng dụng mẫu dùng C++ Application Framework với Qt6/QML. Đây là composition example để kiểm tra runtime, built-in module và dynamic plugin; không phải UI production hoàn chỉnh.

Qt6 chỉ xuất hiện ở tầng application. `core/`, `services/`, runtime contract và plugin SDK không phụ thuộc Qt6.

## 1. Phạm vi và luồng chạy

```text
QML Main.qml
    |
    | Q_PROPERTY / Q_INVOKABLE / signals
    v
RuntimeBridge : QObject
    |
    +-- Runtime
          |
          +-- ModuleManager
                +-- ExampleModule (built-in)
                +-- Example plugin (dynamic library)
```

Các file chính:

- [`main.cpp`](../apps/qt6_app/main.cpp): tạo `QGuiApplication`, QML engine và load module `Framework.Qt6App`.
- [`Main.qml`](../apps/qt6_app/Main.qml): test console và trạng thái lifecycle.
- [`runtime_bridge.h`](../apps/qt6_app/adapters/runtime_bridge.h): QObject/QML contract.
- [`runtime_bridge.cpp`](../apps/qt6_app/adapters/runtime_bridge.cpp): composition, plugin loading và điều khiển runtime.
- [`CMakeLists.txt`](../apps/qt6_app/CMakeLists.txt): QML module, liên kết framework và deploy plugin.

Khi người dùng nhấn **Start runtime**, bridge thực hiện:

1. đăng ký `ExampleModule` nếu runtime mới chưa có module;
2. tìm `framework_example_plugin.*` trong `<application-dir>/plugins`;
3. gọi `PluginLoader::load()`;
4. đăng ký `LoadedPlugin` qua `ModuleManager::registerPlugin()`;
5. gọi `Runtime::initialize()` rồi `Runtime::start()`;
6. cập nhật trạng thái UI thành `Running`.

## 2. Qt entry point

`main.cpp` chỉ chịu trách nhiệm cho Qt và QML:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("Framework.Qt6App", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return application.exec();
}
```

Runtime được sở hữu bởi `RuntimeBridge`, không phải biến global trong `main.cpp`. Cách này làm rõ lifetime và bảo đảm runtime được dừng trước khi bridge bị hủy.

## 3. RuntimeBridge contract

Bridge dùng `QML_ELEMENT` để QML tạo trực tiếp object và expose các property sau:

```cpp
Q_PROPERTY(QString state READ state NOTIFY stateChanged)
Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
Q_PROPERTY(bool moduleRegistered READ moduleRegistered
           NOTIFY moduleRegisteredChanged)
Q_PROPERTY(QString exampleModuleState READ exampleModuleState
           NOTIFY exampleModuleStateChanged)
Q_PROPERTY(bool pluginLoaded READ pluginLoaded NOTIFY pluginLoadedChanged)
```

Các command từ QML:

```cpp
Q_INVOKABLE void start();
Q_INVOKABLE void stop();
Q_INVOKABLE void reset();
Q_INVOKABLE void clearError();
```

Ý nghĩa các property:

- `state`: trạng thái điều phối của runtime, hiện dùng `Stopped` và `Running` ở UI.
- `moduleRegistered`: `ExampleModule` đã được đăng ký vào `ModuleManager` trong runtime hiện tại hay chưa.
- `exampleModuleState`: state thật của module `example`, ví dụ `Initialized`, `Running` hoặc `Stopped`.
- `pluginLoaded`: plugin đã load/register trong runtime hiện tại hay chưa. Sau `Runtime::stop()`, plugin bị unload nên property trở về `false`.
- `lastError`: message của lỗi gần nhất; `errorOccurred` được phát khi có lỗi.

UI nên đọc property và phản ứng theo signal, không tự giữ bản sao lifecycle state.

## 4. Start, Stop và Reset

### Start

`start()` đăng ký built-in module và plugin trước khi gọi lifecycle runtime. Nếu không tìm thấy plugin trong thư mục cạnh executable, bridge trả lỗi và không initialize runtime.

Một runtime đã Stop có thể Start lại:

```text
Start -> initialize -> start
Stop  -> stop -> unload plugin
Start -> load plugin -> initialize -> start
```

`ExampleModule` built-in vẫn nằm trong `ModuleManager` ở state `Stopped`, vì vậy lần Start tiếp theo không đăng ký lại module này. Plugin dynamic được unload khi Stop nên phải load/register lại.

### Stop

`Runtime::stop()` stop module theo thứ tự ngược dependency, sau đó gọi `unloadAllPlugins()`. Bridge đồng bộ `pluginLoaded` về `false` và hiển thị runtime/module ở state `Stopped` nếu không có lỗi.

Plugin mẫu không tự ghi log trong `stop()`. Dòng log `Stopping` hiện tại đến từ `ExampleModule`; dòng `Unloading plugin module: example.plugin` xác nhận bước unload dynamic library.

### Reset

`reset()` dừng runtime hiện tại, tạo `Runtime` mới và đặt lại:

```text
Runtime: Stopped
ExampleModule: Not registered
Example plugin: Not loaded
```

Reset phục vụ test console/retry. Application production thường nên có lifecycle owner và policy retry riêng thay vì expose reset trực tiếp cho người dùng.

## 5. UI hiện tại

`Main.qml` hiển thị ba tín hiệu độc lập:

```qml
Label {
    text: runtimeBridge.moduleRegistered
        ? runtimeBridge.exampleModuleState
        : "Not registered"
}

Label {
    text: runtimeBridge.pluginLoaded ? "Loaded" : "Not loaded"
}
```

Các trạng thái kỳ vọng:

| Thao tác | Runtime | ExampleModule | Example plugin |
| --- | --- | --- | --- |
| Mở app | `Stopped` | `Not registered` | `Not loaded` |
| Start | `Running` | `Running` | `Loaded` |
| Stop | `Stopped` | `Stopped` | `Not loaded` |
| Start lại sau Stop | `Running` | `Running` | `Loaded` |
| Reset | `Stopped` | `Not registered` | `Not loaded` |

Phần Diagnostics hiển thị `lastError` nếu có lỗi, nếu không thì hiển thị thông báo sẵn sàng. QML không truy cập trực tiếp `ModuleManager`, service hoặc `ErrorCode`.

## 6. QML module và CMake

QML module dùng URI `Framework.Qt6App`, version `1.0`, và đăng ký `RuntimeBridge` qua `QML_ELEMENT`:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_add_executable(framework_qt6_app
    main.cpp
)

qt_add_qml_module(framework_qt6_app
    URI Framework.Qt6App
    VERSION 1.0
    SOURCES
        adapters/runtime_bridge.cpp
        adapters/runtime_bridge.h
    QML_FILES
        Main.qml
)

target_link_libraries(framework_qt6_app PRIVATE
    framework_runtime
    framework_example_module
    Qt6::Quick
)
```

Khi `framework_example_plugin` tồn tại, CMake:

1. tạo `<target-dir>/plugins`;
2. copy `$<TARGET_FILE:framework_example_plugin>` vào đó;
3. để bridge tìm plugin bằng `QCoreApplication::applicationDirPath()`.

Nếu build Qt6 không có plugin target, configure vẫn tiếp tục nhưng app sẽ báo plugin không tìm thấy khi nhấn Start. Build mẫu nên bật cả `BUILD_QT6=ON` và `BUILD_PLUGINS=ON`.

## 7. Configure, build và chạy

Ví dụ Windows/MSVC với Qt tại `C:/Qt/6.11.2/msvc2022_64`:

```powershell
cmake -S . -B build-qt `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DBUILD_QT6=ON `
    -DBUILD_PLUGINS=ON `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64

cmake --build build-qt `
    --config Debug `
    --target framework_qt6_app `
    --parallel

.\build-qt\apps\qt6_app\Debug\framework_qt6_app.exe
```

Sau build, plugin mẫu cần có tại:

```text
build-qt/apps/qt6_app/Debug/plugins/framework_example_plugin.dll
```

Trên nền tảng khác, tên file dynamic library thay đổi theo toolchain; code tìm bằng pattern `framework_example_plugin.*`.

## 8. Qt deployment trên Windows

Post-build command gọi `windeployqt` nếu tìm thấy công cụ trong Qt installation. Công cụ này deploy Qt DLL, QML imports và Qt plugins cạnh executable.

Nếu cần chạy thủ công:

```powershell
C:/Qt/6.11.2/msvc2022_64/bin/windeployqt.exe `
    --qmldir apps/qt6_app `
    build-qt/apps/qt6_app/Debug/framework_qt6_app.exe
```

Plugin framework mẫu được copy bởi CMake vào thư mục `plugins` riêng; không commit DLL hoặc các artifact deploy vào source tree.

## 9. Mở rộng thành application thật

Giữ `RuntimeBridge` mỏng: bridge chuyển thao tác UI thành command/use case và chuyển state/error thành view model. Business behavior nên nằm trong application module; HTTP, audio, database và Qt adapter nên nằm ở tầng application/services phù hợp.

```text
QML view
    -> UI bridge/view model
        -> Runtime / ModuleManager
            -> application module
                -> service interfaces
                    -> platform adapters
```

Khi thêm module hoặc plugin:

- module khai báo `ModuleInfo` và dependency rõ ràng;
- đăng ký module trước `Runtime::initialize()`;
- plugin được load bằng `PluginLoader`, sau đó register bằng `registerPlugin`;
- không để QML phụ thuộc implementation của runtime;
- giữ Qt khỏi `core/`, service interfaces và plugin ABI nếu không thật sự cần.

## 10. Checklist audit

- [ ] `engine.loadFromModule()` khớp URI/version của `qt_add_qml_module`.
- [ ] `RuntimeBridge` có owner rõ ràng cho `Runtime`.
- [ ] QML chỉ dùng `Q_PROPERTY`, `Q_INVOKABLE` và signal public.
- [ ] `moduleRegistered` không được dùng thay cho lifecycle state.
- [ ] `exampleModuleState` được cập nhật sau Start, Stop và Reset.
- [ ] Plugin nằm trong `<application-dir>/plugins` trước khi nhấn Start.
- [ ] Stop unload plugin trước khi dynamic library bị đóng.
- [ ] `BUILD_PLUGINS=ON` được dùng khi muốn chạy example plugin.
- [ ] `windeployqt` được chạy cho bản Windows phân phối.

## Tài liệu liên quan

- [Kiến trúc framework](../ARCHITECTURE.md)
- [Hướng dẫn build Qt6 app](qt6-app-build.md)
- [Plugin SDK](../plugins/plugin_sdk/plugin_api.h)
