Runtime Service Injection


Runtime nên trở thành một runtime thuần orchestration, không tự tạo service implementation. Composition root cung cấp toàn bộ services.


IConfig cần chuyển từ read-only abstraction → read/write abstraction; còn persistence là trách nhiệm của implementation/storage, không hard-code vào IConfig.


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