# Architecture

## 1. Mục tiêu

Framework cung cấp một nền tảng C++20 nhỏ, có thể dùng làm điểm khởi đầu cho desktop application hoặc service. Trọng tâm là:

- dependency direction rõ ràng;
- module lifecycle có kiểm soát;
- service contract có thể thay implementation;
- plugin boundary ổn định qua C ABI;
- ownership và cleanup an toàn khi unload dynamic library.

Application cụ thể vẫn chịu trách nhiệm composition root, business feature, persistence/network adapter và UI.

## 2. Dependency direction

```text
Application / adapters
        |
        v
Runtime / ModuleManager ----> Modules and plugins
        |
        v
Services
        |
        v
Core
```

`framework_core` không biết Qt, runtime, plugin, database hay network. `framework_services` phụ thuộc core. `framework_runtime` chỉ phụ thuộc các service contracts và plugin SDK; nó không phụ thuộc các default service implementations. Qt6 chỉ xuất hiện ở `apps/qt6_app`.

## 3. Các target chính

### Core

`framework_core` là interface library chứa `Result<T>`, `Error`, các type và interface nền tảng. Tầng này không sở hữu service hoặc thread.

### Services

`framework_services` cung cấp interface và implementation mặc định cho:

- `ILogger`;
- `IConfig`;
- `IEventBus`;
- `ICommandBus`;
- `IScheduler`;
- `IStorage`;
- `IDiagnostics`.

`IStorage` là key/value generic. Application hoặc module sở hữu namespace, serialization và schema payload; `InMemoryStorage` và `FileStorage` chỉ lưu opaque string values.

Application composition root tạo và chọn service implementations rồi đưa chúng vào `RuntimeContext`. Các reference trong context là non-owning; mọi service phải sống lâu hơn runtime, module và plugin sử dụng chúng.

#### IConfig read/write

`IConfig` là typed read/write contract cho `string`, `int64_t`, `bool` và `double`. Contract chỉ mô tả việc đọc/ghi cấu hình:

- key rỗng trả `InvalidArgument`;
- key không tồn tại trả `NotFound`;
- đọc sai kiểu trả lỗi, không conversion ngầm;
- `set*()` ghi đè và có hiệu lực ở lần đọc tiếp theo.

`IConfig` không chứa file path, format, database handle hoặc persistence policy. `InMemoryConfig` phù hợp cho test/example; `FileConfig` là implementation persistent do composition root chọn và tự chịu trách nhiệm load/persist.

#### IStorage generic và persistent

`IStorage` chỉ là key/value contract với `std::string`. Nó không biết device, sensor, event hay schema domain. `InMemoryStorage` là volatile implementation; `FileStorage` là local persistent implementation. Application chọn backend và truyền `IStorage&` qua `RuntimeContext`.

`FileStorage::load()` được gọi explicit tại composition root. `set()` ghi đồng bộ qua temporary file và replacement, chỉ trả success sau khi persist hoàn tất; lỗi persistence được trả về `core::Result`. Implementation dùng mutex để bảo vệ map và công bố thread-safe cho `get()`/`set()`.

### Runtime

`Runtime` chỉ giữ non-owning service references và sở hữu `ModuleManager`. Nó điều phối:

```text
initialize -> start -> stop
```

`Runtime::stop()` dừng module rồi unload toàn bộ plugin. Runtime có thể được start/stop lại; module built-in giữ trong manager ở state `Stopped`, còn plugin dynamic được load lại sau mỗi stop.

Runtime không tạo, sở hữu hoặc tự chọn service implementation. Application phải tạo service trước, truyền `RuntimeContext`, sau đó giữ các service sống cho đến khi `Runtime::stop()` hoàn tất.

### Modules

Module implement `runtime::IModule`:

```cpp
virtual const ModuleInfo& info() const = 0;
virtual ModuleState state() const = 0;
virtual core::Result<void> initialize() = 0;
virtual core::Result<void> start() = 0;
virtual core::Result<void> stop() = 0;
```

Module sở hữu state nghiệp vụ của mình và phải hoàn tất state transition trước khi trả success.

## 4. Module lifecycle

```text
Discovered -> Loaded -> Initialized -> Started -> Running
Running -> Stopping -> Stopped
Stopped -> Initialized
Plugin: Stopped -> Unloaded
```

Contract hiện tại:

| Operation | State hợp lệ | Kết quả |
| --- | --- | --- |
| Register | `Discovered`, `Loaded` | Module được đưa vào manager |
| Initialize | `Discovered`, `Loaded`, `Stopped` | `Initialized` |
| Start | `Initialized` | `Started` hoặc `Running` |
| Stop | `Started`, `Running` | `Stopped` |
| Unload plugin | `Discovered`, `Loaded`, `Initialized`, `Stopped` | Module bị remove, library bị đóng |

`ModuleManager` resolve dependency trước khi initialize/start, dùng thứ tự ngược khi stop, từ chối missing/self/circular dependency và rollback module đã start nếu module sau đó thất bại.

## 5. Ownership và cleanup

Built-in module được giữ bằng `std::unique_ptr<IModule>` trong `ModuleManager`.

Plugin được giữ qua ba lớp liên quan:

1. `LoadedPlugin` giữ module pointer, destroy function và dynamic-library handle.
2. `PluginModuleProxy` đưa module plugin vào cùng `IModule` contract với built-in module.
3. `ModuleManager::pluginOwners_` giữ `LoadedPlugin` cho đến khi plugin được unload.

Thứ tự cleanup bắt buộc:

```text
stop module
    -> destroy module bằng destroy_plugin_module()
    -> đóng dynamic library
```

Plugin không được giữ callback, scheduler task, function pointer hoặc data pointer trỏ vào library sau khi unload.

## 6. Plugin ABI

Plugin SDK nằm ở `plugins/plugin_sdk/plugin_api.h`. Plugin phải export:

```cpp
extern "C" const PluginDescriptor* get_plugin_descriptor() noexcept;
extern "C" runtime::IModule* create_plugin_module(const runtime::RuntimeContext*) noexcept;
extern "C" void destroy_plugin_module(runtime::IModule*) noexcept;
```

`PluginDescriptor` chứa:

- plugin ID và name;
- semantic version;
- API version và ABI version;
- dependency pointer/count.

`PluginLoader` validate:

- dynamic library load được;
- đủ ba export bắt buộc;
- descriptor hợp lệ, ID/name không rỗng;
- API/ABI tương thích;
- dependencies không null, không rỗng và không trùng;
- module ID khớp descriptor ID;
- module tạo được và không null.

Application chọn path và thời điểm load. Framework không tự động scan thư mục plugin.

## 7. Dependency và lifecycle orchestration

`ModuleInfo::dependencies` chứa ID module mà module hiện tại yêu cầu. Manager kiểm tra toàn bộ dependency trước khi chạy lifecycle. Dependency được initialize/start trước dependent module và stop sau dependent module.

Application composition điển hình:

```text
create and own service implementations
        -> choose InMemoryStorage or FileStorage
        -> create RuntimeContext with non-owning references
        -> create Runtime
    -> register built-in modules
        -> PluginLoader::load(path, runtime.context())
    -> ModuleManager::registerPlugin(...)
    -> Runtime::initialize()
    -> Runtime::start()
    -> application loop
    -> Runtime::stop()
```

Nếu một bước đăng ký, initialize hoặc start lỗi, application phải giữ và báo `Result` thay vì bỏ qua. `Runtime` log lỗi lifecycle qua `ILogger`.

## 8. Error handling

Operational failure dùng `core::Result<T>` và `core::Error`. Các nhóm lỗi thường gặp gồm:

- `InvalidArgument`: contract hoặc descriptor không hợp lệ;
- `NotFound`: module/dependency/plugin path không tồn tại;
- `AlreadyExists`: ID module bị trùng;
- `PluginLoadFailed`: không load được library, symbol hoặc version;
- `StateError`: lifecycle transition không hợp lệ.

Không dùng exception làm flow control cho registration hoặc lifecycle.

## 9. Qt6 composition example

`apps/qt6_app` là adapter tùy chọn, không phải một phần bắt buộc của runtime. `RuntimeBridge` expose runtime state và các thao tác start/stop/reset cho QML. App mẫu:

- đăng ký `ExampleModule` built-in;
- load `framework_example_plugin` từ `plugins` cạnh executable;
- chọn `FileConfig` và `FileStorage` tại composition root của adapter;
- gọi `load()` trước khi runtime sử dụng persistent services;
- hiển thị state runtime, module và plugin;
- chứng minh chu kỳ Start -> Stop -> Start và Reset.

Build Qt6 cần `BUILD_QT6=ON`, `BUILD_PLUGINS=ON` và `CMAKE_PREFIX_PATH` trỏ tới Qt installation. Chi tiết nằm trong `docs/qt6-app-build.md`.

## 10. Cấu hình build

Root CMake cung cấp các option:

```text
BUILD_PLUGINS       Build plugin mẫu và fixture lỗi
BUILD_QT6           Build Qt6/QML example
ENABLE_WARNINGS     Bật warnings nghiêm ngặt
ENABLE_CLANG_TIDY   Chạy clang-tidy
ENABLE_SANITIZERS   Bật ASan/UBSan trên compiler hỗ trợ
```

Các target framework luôn được khai báo từ `core`, `services`, `runtime` và `modules/example_module`. Plugin và Qt6 là phần tùy chọn.

## 11. Quy tắc mở rộng

Khi thêm application:

1. Đặt composition và adapter trong tầng application, không đưa Qt/network/database vào core.
2. Đăng ký module ở composition root.
3. Dùng service interface thay vì phụ thuộc implementation mặc định.
4. Chọn `InMemoryStorage` cho test/example hoặc `FileStorage`/adapter khác cho deployment tại composition root.
5. Gán ID module ổn định và khai báo dependency explicit.
6. Với plugin, dùng đúng SDK, truyền `RuntimeContext` lúc tạo module và giữ allocator boundary qua destroy function.
7. Stop/unsubscribe/cancel mọi callback hoặc task trước khi unload plugin và destroy service.

Khi thêm framework capability dùng chung, cập nhật contract tương ứng và kiểm tra ảnh hưởng tới lifecycle, ownership và ABI trước khi mở rộng adapter.
