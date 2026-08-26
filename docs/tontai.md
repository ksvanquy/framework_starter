Tóm lại: giữ IStorage generic, giữ InMemoryStorage, thêm implementation persistent và cho application chọn implementation.

Tóm lại :

IConfig
→ "App phải được cấu hình thế nào?"

IStorage
→ "App cần nhớ dữ liệu gì?"

Runtime Injection
→ "App muốn dùng implementation nào?"


IDiagnostics nên là generic framework service:

Framework
   ↓
IDiagnostics
   ├── Status
   ├── Health
   ├── Metrics
   ├── Warnings
   └── Errors
IConfig
Thêm write contract:
setString()
setInt()
setBool()
setDouble()
Không bắt IConfig tự lo persistence.

IStorage
Giữ interface key/value generic.
Thêm implementation persistent generic.
InMemoryStorage vẫn giữ cho test/example.

IDiagnostics
Cho phép app/module/plugin đăng ký:
status
health
metrics
warnings
errors
Không thêm field kiểu deviceCount.

tóm lại 
FW cần sửa:

Runtime       → service injection
IConfig       → write API
IStorage      → persistent implementation
IDiagnostics  → extensibility
Tests/Docs    → cập nhật theo API mới

ICommandBus   → chỉ document lifetime, chưa cần đổi API


1. Runtime Injection
→ IoT chọn implementation muốn dùng.

2. IConfig
→ Lưu/đọc cấu hình:
   AC target = 24°C
   polling = 1s
   automation enabled = true

3. IStorage
→ Lưu dữ liệu lâu dài:
   device state
   sensor history
   events
   automation rules

4. IDiagnostics
→ Theo dõi tình trạng app:
   devices online/offline
   scheduler health
   plugin status
   warnings/errors
   metrics


## Runtime Service Injection đã triển khai

### Mục tiêu

Biến `Runtime` thành lớp thuần orchestration:

- điều phối lifecycle `initialize -> start -> stop`;
- quản lý module và plugin;
- ghi log thông qua interface `ILogger` được inject;
- không include hoặc khởi tạo service implementation cụ thể;
- không sở hữu lifetime của các service trong `RuntimeContext`.

Composition root của application sẽ tạo, sở hữu và cung cấp toàn bộ services:

```text
Composition root
   |-- ILogger implementation
   |-- IConfig implementation
   |-- IEventBus implementation
   |-- ICommandBus implementation
   |-- IScheduler implementation
   |-- IStorage implementation
   |-- IDiagnostics implementation
   '-- RuntimeContext -> Runtime
```

### 1. Chốt contract và lifetime

Giữ `RuntimeContext` làm dependency bundle của runtime, gồm các interface:

```cpp
struct RuntimeContext {
   services::ILogger& logger;
   services::IEventBus& eventBus;
   services::IConfig& config;
   services::ICommandBus& commandBus;
   services::IScheduler& scheduler;
   services::IStorage& storage;
   services::IDiagnostics& diagnostics;
};
```

Quy ước ownership:

- `RuntimeContext` chỉ chứa non-owning references;
- composition root tạo và sở hữu các service;
- mọi service phải sống lâu hơn `Runtime` và module/plugin sử dụng service đó;
- application phải stop/unload runtime trước khi destroy service;
- không dùng global singleton để tìm service.

### 2. Contract Runtime

Runtime không còn constructor mặc định tự tạo service. API hiện tại nhận context:

```cpp
Runtime();
```

bằng constructor nhận services:

```cpp
explicit Runtime(RuntimeContext services);
```

`Runtime::Impl` chỉ còn giữ:

```cpp
RuntimeContext context;
ModuleManager moduleManager;
bool initialized;
bool started;
```

Xóa khỏi runtime toàn bộ member concrete:

- `ConsoleLogger`;
- `InMemoryConfig`;
- `InMemoryEventBus`;
- `InMemoryCommandBus`;
- `ThreadScheduler`;
- `InMemoryStorage`;
- `BasicDiagnostics`.

`runtime/src/runtime.cpp` không được include `services/default_services.h` hoặc trực tiếp gọi constructor của các implementation trên. `Runtime` chỉ dùng `context.logger` để ghi lifecycle log và truyền logger vào `ModuleManager`.

### 3. Application và composition root

`Application` nhận `RuntimeContext`; composition root tạo service trước runtime:

```cpp
explicit Application(RuntimeContext services);
```

Composition root cần tạo service trước runtime, ví dụ:

```cpp
services::ConsoleLogger logger(std::clog);
services::InMemoryConfig config;
services::InMemoryEventBus eventBus;
services::InMemoryCommandBus commandBus;
services::ThreadScheduler scheduler;
services::InMemoryStorage storage;
services::BasicDiagnostics diagnostics;

runtime::RuntimeContext context{
   logger, eventBus, config, commandBus,
   scheduler, storage, diagnostics
};

runtime::Runtime runtime(context);
```

Default services vẫn được giữ trong `framework_services` để làm implementation mẫu/test. Chúng không được chuyển ngược vào `framework_runtime`.

### 4. Plugin context injection

Factory hiện tại nhận context:

```cpp
IModule* create_plugin_module(const RuntimeContext* context) noexcept;
```

`PluginLoader::load()` nhận `RuntimeContext&` và truyền context vào factory. Plugin lấy các interface cần thiết từ context và inject tiếp vào constructor của module.

Yêu cầu lifetime:

- plugin không được giữ `RuntimeContext*` sau khi runtime bị destroy;
- module phải stop trước khi unload plugin;
- plugin phải unload trước khi composition root destroy service;
- cần document rõ ABI contract vì `RuntimeContext` chứa C++ references.

Nếu cần ABI ổn định giữa compiler/toolchain khác nhau, tạo một plugin service table dùng opaque handles/function pointers ở giai đoạn sau. Không đưa thay đổi ABI đó vào cùng đợt refactor đầu tiên nếu chưa cần thiết.

### 5. Coverage cần duy trì

Các test/validation của `framework_runtime` cần tiếp tục kiểm tra:

1. `Runtime` khởi tạo bằng custom/fake implementations.
2. Lifecycle log đi qua `ILogger` được inject.
3. Runtime không cần `services/default_services.h` để compile và link.
4. Module nhận đúng logger/event bus từ composition root.
5. Runtime không tạo service implementation ngoài context.
6. Plugin factory nhận và sử dụng context được truyền vào.
7. Plugin được stop/unload trước khi service dependencies bị hủy.
8. Runtime vẫn xử lý đúng lifecycle idempotency và lỗi module hiện tại.

### 6. Tài liệu và build contract

- Bảo đảm `framework_runtime` chỉ phụ thuộc các service interfaces cần thiết.
- Không thêm dependency từ runtime tới default service implementations.
- Giữ test trong build mặc định khi test infrastructure được bổ sung.
- Cập nhật `ARCHITECTURE.md` để mô tả `Runtime` không sở hữu services.
- Cập nhật `README.md` với ví dụ composition root mới.
- Cập nhật plugin SDK documentation với factory signature và lifetime contract.
- Cập nhật tài liệu ứng dụng sau khi API mới được chốt.
- Bỏ qua `apps/qt6_app` trong đợt triển khai đầu tiên; chỉ xử lý adapter đó ở migration phase riêng nếu cần.

### Trạng thái triển khai

Các bước Runtime Service Injection ở trên đã được áp dụng. Khi mở rộng framework, chỉ thêm implementation ở composition root và giữ `framework_runtime` phụ thuộc contract.

### Tiêu chí hoàn tất

- `Runtime` không còn constructor mặc định tự tạo services.
- `runtime/src/runtime.cpp` không include hoặc instantiate concrete service implementation.
- Mọi service implementation được tạo tại composition root.
- Built-in module và plugin nhận dependency từ bên ngoài.
- Runtime chỉ điều phối lifecycle, module và plugin.
- Có test chứng minh implementation có thể được thay thế.
- Tài liệu không còn mô tả Runtime là owner của service instances.


## Kế hoạch triển khai IConfig Read/Write

### Mục tiêu

Chuyển `IConfig` từ abstraction chỉ đọc thành contract đọc/ghi cấu hình, nhưng không biến `IConfig` thành persistence layer:

- `IConfig` trả lời application cần cấu hình như thế nào;
- implementation của `IConfig` quyết định dữ liệu được lưu ở đâu và bằng cách nào;
- runtime, module và plugin chỉ phụ thuộc `IConfig` interface;
- application/composition root chọn implementation phù hợp với môi trường chạy.

### 1. Chốt API của IConfig

Mở rộng `IConfig` với các thao tác đọc/ghi typed:

```cpp
class IConfig {
public:
   virtual ~IConfig() = default;

   virtual core::Result<std::string> getString(const std::string& key) const = 0;
   virtual core::Result<int64_t> getInt(const std::string& key) const = 0;
   virtual core::Result<bool> getBool(const std::string& key) const = 0;
   virtual core::Result<double> getDouble(const std::string& key) const = 0;

   virtual core::Result<void> setString(std::string key, std::string value) = 0;
   virtual core::Result<void> setInt(std::string key, int64_t value) = 0;
   virtual core::Result<void> setBool(std::string key, bool value) = 0;
   virtual core::Result<void> setDouble(std::string key, double value) = 0;
};
```

Contract cần thống nhất thêm:

- key rỗng trả về `InvalidArgument`;
- key không tồn tại trả về `NotFound`;
- đọc sai kiểu trả về lỗi rõ ràng, không âm thầm ép kiểu;
- ghi đè cùng key được phép;
- `set*()` cập nhật giá trị có hiệu lực cho lần đọc tiếp theo;
- thread-safety phải được document theo từng implementation, không giả định trong interface;
- chưa thêm `flush()`, `save()` hoặc đường dẫn file vào `IConfig`.

### 2. Tách config contract khỏi persistence

`IConfig` chỉ định nghĩa thao tác đọc/ghi. Persistence là trách nhiệm của implementation:

```text
IConfig
   |-- InMemoryConfig       -> process memory, test/example
   |-- FileConfig            -> file format/path do application chọn
   |-- DatabaseConfig        -> database adapter nếu cần
   '-- RemoteConfig          -> remote configuration service nếu cần
```

Không đưa các chi tiết sau vào `IConfig`:

- file path;
- JSON/YAML/INI format;
- database connection;
- auto-save policy;
- transaction hoặc migration của storage;
- dependency tới filesystem, database hay network.

Implementation persistent có thể tự động load khi khởi tạo và persist khi `set*()` thành công, hoặc cung cấp policy riêng ở ngoài interface. Policy đó phải được document và test ở implementation tương ứng.

### 3. Cập nhật InMemoryConfig

Giữ `InMemoryConfig` làm implementation mặc định cho test/example:

- bổ sung storage cho `int64_t`, `bool` và `double`, hoặc dùng một value type nội bộ có tag kiểu;
- giữ dữ liệu trong memory, không đọc/ghi filesystem;
- bảo đảm các getter/setter tuân thủ cùng error contract với implementation persistent;
- nếu hỗ trợ concurrent access thì bảo vệ cả getter và setter bằng cùng synchronization policy;
- không để `Runtime` biết hoặc phụ thuộc vào implementation này.

Nên dùng một representation typed thống nhất thay vì chuyển mọi giá trị qua `std::string`, nhằm tránh mất độ chính xác và tránh conversion ngầm.

### 4. Persistent implementation

Implementation persistent `FileConfig` nằm ở tầng services và không sửa trách nhiệm của `IConfig`:

- application truyền storage/path/options vào constructor;
- implementation load cấu hình theo format đã chọn;
- implementation persist theo policy đã chọn;
- lỗi I/O, parse, permission hoặc schema phải được chuyển thành `core::Error` phù hợp;
- ghi dữ liệu phải bảo đảm không tạo file cấu hình bị hỏng khi tiến trình dừng giữa chừng;
- không làm thay đổi API của `Runtime`.

Workspace hiện có `FileConfig` tối giản cho key/value typed. Database hoặc remote config chỉ thêm khi có use case cụ thể, không đưa dependency nặng vào `framework_services` mặc định.

### 5. Migration call sites

Rà soát module, plugin và application để:

1. thay các giả định config read-only bằng API `set*()` khi cần cập nhật cấu hình runtime;
2. dùng đúng getter theo kiểu dữ liệu;
3. xử lý `NotFound` và type mismatch thay vì dùng giá trị mặc định im lặng;
4. không tự ghi file/database từ module khi đã có `IConfig` implementation;
5. để composition root chọn `InMemoryConfig` cho test/example và persistent config cho production.

Kết quả rà soát hiện tại: Qt composition root sử dụng `FileConfig`, gọi `load()`, rồi dùng `getBool()`/`setBool()` cho các cờ enable module/plugin. Module/plugin mới vẫn phải phụ thuộc `IConfig`, không phụ thuộc trực tiếp `FileConfig`, và phải xử lý lỗi theo contract bên trên.

Các key cấu hình nên được đặt tên ổn định, document namespace và validate ở application/module, ví dụ:

```text
ac.target_celsius       -> double
polling.interval_ms     -> int
automation.enabled      -> bool
```

### 6. Test bắt buộc

Tạo test contract dùng chung cho mọi `IConfig` implementation:

1. Set/get thành công cho string, int, bool và double.
2. Ghi đè key trả về giá trị mới.
3. Key không tồn tại trả về `NotFound`.
4. Key rỗng bị từ chối.
5. Đọc bằng sai kiểu trả về lỗi, không conversion ngầm.
6. `InMemoryConfig` không tạo hoặc phụ thuộc filesystem.
7. Persistent implementation giữ dữ liệu sau khi destroy/recreate object.
8. Lỗi parse và lỗi I/O được trả về qua `core::Result`.
9. Runtime/module có thể dùng custom `IConfig` mà không include default implementation.
10. Concurrent behavior được kiểm tra nếu implementation công bố thread-safe.

### 7. Cập nhật CMake và tài liệu

- Duy trì header/source `IConfig`, `InMemoryConfig` và `FileConfig` trong các target phù hợp.
- Đặt persistent implementation ở target riêng nếu cần dependency filesystem/database.
- Không link persistence dependency vào `framework_runtime`.
- Cập nhật `README.md` với ví dụ chọn `InMemoryConfig` hoặc persistent config tại composition root.
- Cập nhật `ARCHITECTURE.md` để phân biệt config contract và persistence implementation.
- Document typed API, error contract, thread-safety và persistence policy.
- Cập nhật tài liệu khi contract hoặc persistence policy thay đổi.

### Trạng thái triển khai

Typed API, `InMemoryConfig`, `FileConfig` và Qt composition call sites đã được cập nhật. Test contract dùng chung và các test lỗi I/O vẫn là phần coverage cần bổ sung khi test infrastructure được mở rộng.

### Tiêu chí hoàn tất

- `IConfig` hỗ trợ đọc/ghi string, int, bool và double.
- Getter/setter có error contract nhất quán.
- `IConfig` không chứa file path, database, format hoặc auto-save policy.
- `InMemoryConfig` vẫn dùng được cho test/example.
- Có ít nhất một persistent implementation do application lựa chọn.
- Runtime/module/plugin chỉ phụ thuộc `IConfig` interface.
- Test chứng minh implementation config có thể thay thế mà không sửa runtime.


## Kế hoạch triển khai IStorage Generic và Persistent

### Mục tiêu

Giữ `IStorage` là abstraction key/value generic để framework không phụ thuộc domain IoT hoặc một công nghệ persistence cụ thể:

- `IStorage` lưu dữ liệu cần nhớ lâu dài, không quyết định schema nghiệp vụ;
- `InMemoryStorage` tiếp tục phục vụ test/example và các use case không cần persistence;
- thêm persistent implementation generic ở tầng services/adapter;
- application/composition root chọn implementation theo môi trường chạy;
- runtime, module và plugin chỉ phụ thuộc `IStorage` interface.

### 1. Giữ nguyên storage contract

Giữ API hiện tại ở mức key/value string:

```cpp
class IStorage {
public:
   virtual ~IStorage() = default;

   virtual core::Result<void> set(
      const std::string& key,
      const std::string& value) = 0;

   virtual core::Result<std::string> get(
      const std::string& key) = 0;
};
```

Contract cần thống nhất:

- key rỗng trả về `InvalidArgument`;
- key không tồn tại trả về `NotFound`;
- `set()` ghi đè giá trị hiện có;
- dữ liệu được lưu dưới dạng bytes/string, không đưa `device`, `sensor`, `event` hoặc domain type vào interface;
- không thêm file path, database handle, serialization format hoặc `flush()` vào `IStorage`;
- thread-safety và durability phải được document theo từng implementation.

Nếu cần thao tác xóa, list key hoặc batch transaction trong tương lai, chỉ mở rộng contract khi có use case thực tế và phải đánh giá compatibility trước.

### 2. Giữ và chuẩn hóa InMemoryStorage

`InMemoryStorage` tiếp tục là implementation mặc định cho test/example:

- giữ `std::unordered_map<std::string, std::string>` nội bộ;
- bảo vệ getter/setter bằng mutex;
- trả cùng error code và message contract với persistent implementation;
- không tạo file, không phụ thuộc filesystem/database;
- dữ liệu mất khi object bị hủy, đây là behavior được document rõ;
- không để runtime tự tạo implementation này; composition root quyết định có dùng nó hay không.

### 3. Persistent implementation generic

`FileStorage` là implementation riêng của `IStorage`, không thay đổi contract:

```text
IStorage
   |-- InMemoryStorage  -> test/example, volatile
   |-- FileStorage      -> local persistent key/value
   |-- DatabaseStorage  -> application database adapter
   '-- RemoteStorage    -> remote persistence adapter
```

`FileStorage` hiện là implementation persistent generic tối giản:

- nhận path/options từ constructor;
- tự tạo parent directory nếu cần;
- lưu key/value an toàn với key hoặc value chứa ký tự phân cách, newline và dữ liệu nhị phân được encode;
- load dữ liệu explicit hoặc qua factory trả `core::Result` để lỗi mở/parse không bị che giấu;
- ghi vào file tạm, flush/close thành công rồi replace file chính;
- không làm mất file cũ nếu serialize hoặc ghi file tạm thất bại;
- xác định rõ behavior khi file bị hỏng, file bị khóa hoặc permission bị từ chối.

`FileStorage` không nên chứa schema của device state, sensor history, events hoặc automation rules. Các module/application chịu trách nhiệm namespace và serialize payload của mình trước khi gọi `set()`.

### 4. Persistence và lifetime

Quy ước ownership:

- composition root tạo và sở hữu `IStorage` implementation;
- `RuntimeContext` chỉ giữ reference non-owning;
- runtime/module/plugin phải stop trước khi storage bị destroy;
- persistent storage phải hoàn tất hoặc báo lỗi rõ ràng trước khi application thoát;
- không giữ reference tới buffer tạm hoặc object thuộc storage sau khi call kết thúc;
- nếu implementation có background flush, destructor phải stop và join worker trước khi đóng file/resource.

Durability policy phải được document riêng:

- `set()` thành công có nghĩa dữ liệu đã được cập nhật trong memory hay đã bền vững trên disk;
- nếu chỉ queue ghi bất đồng bộ, cần API/status riêng ở implementation, không giả vờ là durable;
- lỗi persistence không được bị nuốt hoặc chỉ log rồi trả success.

### 5. Chọn implementation tại composition root

Application chọn backend mà không sửa runtime/module contract:

```cpp
services::InMemoryStorage storage; // test/example
// hoặc:
services::FileStorage storage(storagePath); // local deployment
storage.load();

runtime::RuntimeContext context{
   logger, eventBus, config, commandBus,
   scheduler, storage, diagnostics
};
```

Production application có thể thay `FileStorage` bằng database hoặc remote adapter. Framework không tự động chọn backend, không tự dò filesystem và không hard-code path persistence trong `Runtime`.

### 6. Namespace và payload ownership

Không có module/plugin nào trong workspace hiện tại ghi `IStorage`, vì vậy framework chưa định nghĩa domain schema hoặc key cố định. Khi một module/application bắt đầu sử dụng storage, caller đó phải tự quy ước namespace ổn định để tránh collision:

```text
devices/<device-id>/state
sensors/<sensor-id>/history
automation/rules/<rule-id>
events/<event-id>
```

`IStorage` chỉ nhận key/value dạng `std::string` và coi value là payload opaque. Việc serialize/deserialize payload thuộc module hoặc adapter chuyên trách:

```cpp
const std::string key = "devices/" + deviceId + "/state";
const std::string payload = deviceStateSerializer.serialize(state);
const auto result = storage.set(key, payload);
```

Module sở hữu schema payload, schema version và compatibility/migration policy của namespace đó. Có thể đặt version trong payload, ví dụ `{"version":1,...}`, nhưng không thêm field kiểu `deviceCount` hoặc domain type vào `IStorage`. Payload binary cũng phải được truyền dưới dạng bytes trong `std::string`, không qua conversion text tùy tiện.

Namespace không được lấy từ file path hoặc implementation cụ thể; cùng một key phải có ý nghĩa như nhau khi application đổi giữa `InMemoryStorage` và `FileStorage`.

### 7. Coverage cần duy trì

Storage contract tests dùng chung cho mọi implementation cần bao phủ:

1. Set/get value thành công.
2. Ghi đè key trả về giá trị mới.
3. Key không tồn tại trả về `NotFound`.
4. Key rỗng trả về `InvalidArgument`.
5. Key/value chứa newline, tab, unicode và ký tự phân cách không làm hỏng dữ liệu.
6. `InMemoryStorage` không tạo hoặc đọc filesystem.
7. `FileStorage` giữ dữ liệu sau destroy/recreate object.
8. File không tồn tại được xử lý theo policy đã document.
9. File hỏng trả lỗi parse rõ ràng.
10. Lỗi directory, permission, disk/full hoặc rename không trả success giả.
11. Ghi thất bại không làm mất bản snapshot hợp lệ trước đó.
12. Concurrent behavior được kiểm tra nếu implementation công bố thread-safe.

### 8. CMake và tài liệu

- Giữ `IStorage` trong service contract, không thêm dependency persistence vào `framework_runtime`.
- Giữ `InMemoryStorage` trong default services.
- Đặt `FileStorage` ở target riêng nếu cần dependency hoặc policy riêng.
- Duy trì composition root mẫu để cho thấy cách chọn backend.
- Cập nhật `README.md` và `ARCHITECTURE.md` về volatile/persistent behavior.
- Document format, atomic write, durability, error và thread-safety của `FileStorage`.
- Không đưa database/network dependency vào framework mặc định nếu chưa có use case.

### Trạng thái triển khai

`IStorage`, `InMemoryStorage`, `FileStorage`, composition root và tài liệu contract đã được triển khai. Storage contract tests và test lỗi persistence vẫn là coverage cần bổ sung.

### Tiêu chí hoàn tất

- `IStorage` vẫn là key/value generic, không chứa domain schema.
- `InMemoryStorage` vẫn hoạt động cho test/example.
- Có ít nhất một persistent implementation độc lập với `Runtime`.
- Application/composition root chọn được implementation.
- Persistence error và durability behavior được document và test.
- Runtime/module/plugin không biết backend cụ thể.
- Dữ liệu hợp lệ trước đó không bị mất khi một lần ghi mới thất bại.