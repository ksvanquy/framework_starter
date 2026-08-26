# C++ Application Framework Starter

Framework C++20 tối giản để bắt đầu một desktop application hoặc service có module, service và plugin với lifecycle rõ ràng.

Repository này là **starter dùng framework**, không phải một application hoàn chỉnh. Framework cung cấp các contract và implementation nền tảng; application sở hữu composition root, cấu hình module/plugin và giao diện riêng.

## Có gì trong starter

- `framework_core`: `Result<T>`, error, type và interface nền tảng, không phụ thuộc Qt.
- `framework_services`: logger, config, event bus, command bus, scheduler, storage và diagnostics mặc định.
- `framework_runtime`: `Runtime`, `ModuleManager`, lifecycle module và `PluginLoader`.
- `framework_example_module`: built-in module mẫu.
- `plugins/example_plugin`: dynamic plugin mẫu với C ABI.
- `apps/qt6_app`: ứng dụng Qt6/QML mẫu chứng minh runtime plugin loading.

Kiến trúc chi tiết nằm trong [ARCHITECTURE.md](ARCHITECTURE.md). Hướng dẫn build Qt6 nằm trong [docs/qt6-app-build.md](docs/qt6-app-build.md).

## Yêu cầu

- CMake 3.20 trở lên.
- Compiler hỗ trợ C++20.
- Windows với Visual Studio 2022/MSVC, hoặc compiler tương đương trên Linux/macOS.
- Qt 6.11 chỉ cần khi build `qt6_app`.

## Build framework

Từ thư mục gốc:

```powershell
cmake -S . -B build `
    -DBUILD_PLUGINS=ON
cmake --build build --config Debug --parallel
```

Tùy chọn chính:

| Tùy chọn | Mặc định | Ý nghĩa |
| --- | --- | --- |
| `BUILD_PLUGINS` | `ON` | Build plugin mẫu và các plugin fixture kiểm tra lỗi |
| `BUILD_QT6` | `OFF` | Build ứng dụng Qt6/QML mẫu |
| `ENABLE_WARNINGS` | `ON` | Bật cảnh báo nghiêm ngặt; MSVC dùng `/W4 /WX` |
| `ENABLE_CLANG_TIDY` | `OFF` | Chạy clang-tidy khi compile |
| `ENABLE_SANITIZERS` | `OFF` | Bật ASan/UBSan trên compiler hỗ trợ |

## Tạo application

Application thường làm composition root:

1. Chọn và tạo các service implementation.
2. Tạo `RuntimeContext` từ các service đó.
3. Đăng ký built-in module bằng `registerModule(...)`.
4. Load plugin bằng `PluginLoader::load(...)`, sau đó đăng ký bằng `registerPlugin(...)`.
5. Gọi `initialize()`, `start()`, chạy application loop, rồi gọi `stop()`.

Ví dụ rút gọn:

```cpp
#include "services/default_services.h"
#include "services/file_storage.h"
#include "runtime/plugin_loader.h"
#include "runtime/runtime.h"

int main() {
    using namespace framework;

    services::ConsoleLogger logger(std::clog);
    services::InMemoryConfig config;
    services::InMemoryEventBus eventBus;
    services::InMemoryCommandBus commandBus;
    services::ThreadScheduler scheduler;
    services::InMemoryStorage storage; // Use FileStorage for local persistence.
    services::BasicDiagnostics diagnostics;

    runtime::RuntimeContext context{
        logger, eventBus, config, commandBus, scheduler, storage, diagnostics};
    runtime::Runtime runtime(context);

    // Register application-owned built-in modules here.
    // runtime.moduleManager().registerModule(...);

    auto plugin = runtime::PluginLoader{}.load("plugins/my_plugin.dll", runtime.context());
    if (!plugin) return 1;
    if (!runtime.moduleManager().registerPlugin(std::move(plugin.value()))) return 1;

    if (!runtime.initialize()) return 1;
    if (!runtime.start()) return 1;

    // Application loop.

    return runtime.stop() ? 0 : 1;
}
```

Đường dẫn và phần mở rộng dynamic library phải do application quyết định theo platform. Framework không tự quét toàn bộ filesystem.

## Module và plugin

Built-in module implement `runtime::IModule` và được sở hữu bởi `ModuleManager` qua `std::unique_ptr`.

Plugin export ba symbol C ABI:

```cpp
extern "C" const PluginDescriptor* get_plugin_descriptor() noexcept;
extern "C" runtime::IModule* create_plugin_module(
    const runtime::RuntimeContext* context) noexcept;
extern "C" void destroy_plugin_module(runtime::IModule*) noexcept;
```

`PluginLoader::load(path, runtime.context())` truyền `RuntimeContext` vào factory; plugin chỉ được sử dụng context trong lifetime của module.

`PluginLoader` kiểm tra library, symbol, descriptor, API/ABI version, dependency metadata và module ID trước khi trả `LoadedPlugin`. `ModuleManager` giữ module proxy, destroy function và library handle cho đến khi unload.

## Qt6 example

Configure với Qt6:

```powershell
cmake -S . -B build-qt `
    -DBUILD_QT6=ON `
    -DBUILD_PLUGINS=ON `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
cmake --build build-qt --config Debug --target framework_qt6_app --parallel
```

Chạy:

```powershell
.\build-qt\apps\qt6_app\Debug\framework_qt6_app.exe
```

Build sẽ copy `framework_example_plugin.dll` vào `Debug/plugins`. Khi nhấn Start, app load/register plugin mẫu; UI hiển thị state của runtime, built-in module và plugin.

## Phạm vi của framework

Framework không cung cấp sẵn business feature, HTTP client, database implementation, audio, Qt UI framework riêng hoặc cơ chế auto-discovery plugin toàn cục. Những phần đó thuộc application và adapter tương ứng.

Không dùng C++20 modules (`export module`) để chỉ plugin runtime; plugin system của framework là dynamic library với C ABI.

## Cấu trúc thư mục

```text
core/                  Contract và primitive nền tảng
services/              Service interfaces và implementation mặc định
runtime/               Runtime, ModuleManager, PluginLoader
modules/example_module Built-in module mẫu
plugins/plugin_sdk     Plugin ABI header
plugins/example_plugin Plugin mẫu và fixture lỗi
apps/qt6_app/          Qt6/QML composition example
docs/                  Hướng dẫn build và blueprint
```