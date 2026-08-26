# Build và chạy qt6_app

Tài liệu này mô tả cách build và chạy ứng dụng Qt6 mẫu trong `apps/qt6_app`.

## Môi trường

Môi trường được quy định trong `.vscode/c_cpp_properties.json`:

- Windows x64;
- Visual Studio Community 2022;
- MSVC `14.42.34433`;
- C++20;
- Qt `6.11.2` cho MSVC2022 x64;
- Qt components: Core, Qml và Quick.

Các đường dẫn mặc định:

```text
Compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe
Qt:      C:/Qt/6.11.2/msvc2022_64
```

Nếu Visual Studio hoặc Qt được cài ở vị trí khác, thay các đường dẫn tương ứng trong lệnh configure.

## Configure

Mở **Developer PowerShell for VS 2022** hoặc PowerShell đã có `cmake` và compiler MSVC trong `PATH`, sau đó chạy từ thư mục gốc framework:

```powershell
cmake -S . -B build-qt `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DBUILD_QT6=ON `
    -DBUILD_PLUGINS=ON `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
```

`BUILD_QT6=ON` chỉ thêm app Qt6. Core, Services và Runtime vẫn không phụ thuộc Qt6.

## Build

Build riêng app:

```powershell
cmake --build build-qt `
    --config Debug `
    --target framework_qt6_app `
    --parallel
```

Build này cũng build các target framework và plugin cần thiết. Target được định nghĩa tại [apps/qt6_app/CMakeLists.txt](../apps/qt6_app/CMakeLists.txt).

## Chạy

Với cấu hình Debug của Visual Studio:

```powershell
.\build-qt\apps\qt6_app\Debug\framework_qt6_app.exe
```

Ứng dụng hiện hiển thị test console để điều khiển lifecycle của Runtime, `ExampleModule` và plugin mẫu qua `RuntimeBridge`. Khi build với `BUILD_PLUGINS=ON`, plugin được copy vào `Debug/plugins` cạnh executable và được load/register lúc nhấn Start.

## Nếu thiếu Qt DLL

CMake sẽ tự gọi `windeployqt` sau build nếu tìm thấy công cụ này. Kiểm tra:

```powershell
Test-Path C:/Qt/6.11.2/msvc2022_64/bin/windeployqt.exe
```

Nếu cần chạy thủ công:

```powershell
C:/Qt/6.11.2/msvc2022_64/bin/windeployqt.exe `
    --qmldir apps/qt6_app `
    build-qt/apps/qt6_app/Debug/framework_qt6_app.exe
```

Không đưa các DLL được deploy vào source tree. `build-qt/` là thư mục build cục bộ.

## Mã nguồn app

- [main.cpp](../apps/qt6_app/main.cpp): khởi tạo `QGuiApplication` và QML engine.
- [Main.qml](../apps/qt6_app/Main.qml): giao diện test console.
- [runtime_bridge.h](../apps/qt6_app/adapters/runtime_bridge.h): QObject/QML interface.
- [runtime_bridge.cpp](../apps/qt6_app/adapters/runtime_bridge.cpp): điều khiển Runtime và module.

Qt6 app thuộc tầng `apps/`; Qt không được đưa vào Core, Services hoặc Runtime.
