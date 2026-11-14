# IoT Smart Lock System 🚪🔐

Hệ thống khóa cửa thông minh (Smart Lock) ứng dụng IoT, cung cấp các phương thức xác thực đa dạng để kiểm soát truy cập an toàn.

## 🛠 Tính năng chính
* **Mở khóa bằng RFID:** Quản lý thẻ từ với module MFRC522.
* **Mở khóa bằng Vân tay:** Quá trình quét và xác thực sinh trắc học qua cảm biến vân tay.
* **Mở khóa bằng Mật khẩu:** Nhập mật khẩu qua Keypad 4x4, có hệ thống cảnh báo nếu nhập sai quá 3 lần.
* **Màn hình hiển thị:** Tương tác trực quan qua màn hình LCD I2C (16x2).
* **Kết nối IoT:** Tích hợp Blynk (`Smartlock_blynk.ino`) để theo dõi và điều khiển từ xa.

## 💻 Công nghệ và Phần cứng sử dụng
* **Vi điều khiển:** Arduino / ESP8266 / ESP32
* **Cảm biến & Module:**
  * Cảm biến vân tay (Adafruit Fingerprint)
  * Module đọc thẻ RFID RC522
  * Bàn phím ma trận 4x4 Keypad
  * Màn hình LCD 16x2 kèm module giao tiếp I2C
  * Động cơ Servo điều khiển chốt khóa
* **Ngôn ngữ:** C/C++ (Arduino framework)
* **Lưu trữ:** EEPROM để lưu trữ thông tin thẻ và mật khẩu.

## 📂 Cấu trúc mã nguồn
* `SmartLock.ino`: Chứa logic xử lý chính của hệ thống, quản lý ngắt, đọc dữ liệu cảm biến và điều khiển Servo.
* `Smartlock_blynk.ino`: Xử lý giao tiếp mạng và đồng bộ dữ liệu với nền tảng Blynk.

## 🎥 Video Demo Dự Án
[![Smart Lock Demo](https://img.youtube.com/vi/IPrLoBeEPhU&t=60s/0.jpg)](https://www.youtube.com/watch?v=IPrLoBeEPhU&t=60s)
