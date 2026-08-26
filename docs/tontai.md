​1. Mở rộng IConfig (Write & Persistence Contract)
​Write API: Bổ sung các phương thức thiết lập dữ liệu theo kiểu dữ liệu nguyên thủy (string, int, bool, double).
​Persistence Lifecycle: Tích hợp các phương thức chuẩn hóa như save() hoặc flush() vào contract để quản lý việc đồng bộ dữ liệu cấu hình từ bộ nhớ đệm xuống tầng lưu trữ vật lý.
​Notification Mechanism: Hỗ trợ cơ chế thông báo sự kiện (event notification) khi cấu hình thay đổi để các thành phần liên quan kịp thời cập nhật.
​2. Trừu tượng hóa và Inject IStorage (Persistence Support)
​Decoupling Interface & Implementation: Giữ nguyên core contract IStorage (key-value cơ bản) và tách rời hoàn toàn khỏi các implementation trên RAM (InMemoryStorage).
​Persistent Providers: Cung cấp các hiện thực mẫu dựa trên file hoặc cơ sở dữ liệu nhúng có khả năng tự động nạp dữ liệu (load) khi khởi động và ghi nhận khi thay đổi.
​External Injection: Cho phép application đăng ký hoặc tiêm (inject) custom storage implementation vào vòng đời khởi tạo của runtime thông qua RuntimeBuilder hoặc Context.
​3. Thiết kế Mở rộng IDiagnostics (Extensible Diagnostics Model)
​Standard Operational Metrics: Mở rộng DiagnosticSnapshot để bao gồm các chỉ số vận hành tiêu chuẩn: trạng thái các thành phần (runtime, module, plugin), bộ đếm số lượng, trạng thái bộ lập lịch (Scheduler), và danh sách lỗi/cảnh báo gần nhất (recent_errors, recent_warnings).
​Provider Pattern: Xây dựng cơ chế đăng ký linh hoạt (IDiagnosticProvider) cho phép các module/plugin tự động đóng góp các metric tùy chỉnh vào snapshot chung mà không làm ô nhiễm tầng core framework bằng các thuật ngữ nghiệp vụ cụ thể.
​4. Cơ chế Service Injection cho Runtime (Inversion of Control)
​Lightweight DI / Registration: Áp dụng mô hình đăng ký dịch vụ thông qua RuntimeBuilder hoặc RuntimeContext.
​Flexible Initialization Flow: Cho phép application chủ động định nghĩa và đăng ký các service tùy chỉnh trước khi khởi động runtime.
​Safe Fallback: Tự động sử dụng các default service nội bộ làm phương án dự phòng an toàn trong trường hợp application không cung cấp custom implementation.