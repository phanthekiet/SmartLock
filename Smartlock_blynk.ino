#define BLYNK_TEMPLATE_ID "TMPL6IEIclMLF"
#define BLYNK_TEMPLATE_NAME "esp8266"
// !!! NGUY HIEM !!! BAN PHAI TAO LAI (REGENERATE) AUTH TOKEN NAY
#define BLYNK_AUTH_TOKEN "w1AXW8g0ljEoNqjdXASijXcQq0VUlsjc"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// !!! NGUY HIEM !!! BAN PHAI DOI MAT KHAU WIFI NAY
char ssid[] = "vuhung";
char pass[] = "12345678";

// Khởi tạo BlynkTimer
BlynkTimer timer;

// Biến lưu dữ liệu từ Serial
String serialData = "";

// Hàm tắt nút V0 sau 2 giây
void turnOffV0() {
  Blynk.virtualWrite(V0, 0);
}
// Hàm tắt nút V1 sau 2 giây
void turnOffV1() {
  Blynk.virtualWrite(V1, 0);
}
// Hàm tắt nút V2 sau 2 giây
void turnOffV2() {
  Blynk.virtualWrite(V2, 0);
}
// Hàm tắt nút V3 sau 2 giây
void turnOffV3() {
  Blynk.virtualWrite(V3, 0);
}
// [THÊM MỚI] Hàm tắt nút V5
void turnOffV5() {
  Blynk.virtualWrite(V5, 0);
}


// Xử lý nút "Thêm thẻ" từ Virtual Pin V0
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1) {
    Serial.print("A"); // Gửi lệnh 'A'
    Blynk.virtualWrite(V4, "Đã gửi lệnh: Thêm thẻ");
    timer.setTimeout(2000L, turnOffV0);
  }
}

// Xử lý nút "Thêm vân tay" từ Virtual Pin V1
BLYNK_WRITE(V1) {
  int value = param.asInt();
  if (value == 1) {
    Serial.print("B"); // Gửi lệnh 'B'
    Blynk.virtualWrite(V4, "Đã gửi lệnh: Thêm vân tay");
    timer.setTimeout(2000L, turnOffV1);
  }
}

// Xử lý nút "Mở cửa" từ Virtual Pin V2
BLYNK_WRITE(V2) {
  int value = param.asInt();
  if (value == 1) {
    Serial.print("O"); // Gửi lệnh 'O'
    Blynk.virtualWrite(V4, "Đã gửi lệnh: Mở cửa");
    timer.setTimeout(2000L, turnOffV2);
  }
}

// Xử lý nút "Xóa Thẻ" từ Virtual Pin V3
BLYNK_WRITE(V3) {
  int value = param.asInt();
  if (value == 1) {
    Serial.print("D"); // Gửi lệnh 'D' 
    Blynk.virtualWrite(V4, "Đã gửi lệnh: Xóa Thẻ (D)");
    timer.setTimeout(2000L, turnOffV3);
  }
}

// [THÊM MỚI] Xử lý nút "Mở Khóa Hệ Thống" từ Virtual Pin V5
BLYNK_WRITE(V5) {
  int value = param.asInt();
  if (value == 1) {
    Serial.print("U"); // Gửi lệnh 'U' (Unlock)
    Blynk.virtualWrite(V4, "Đã gửi lệnh: MO KHOA HE THONG");
    timer.setTimeout(2000L, turnOffV5);
  }
}


void setup() {
  Serial.begin(9600);
  // *** NHỚ THAY THÔNG TIN WIFI VÀ AUTH TOKEN MỚI VÀO ĐÂY ***
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("ESP8266 đã kết nối Blynk");
}

void loop() {
  Blynk.run();
  timer.run(); 
  
  // Đọc dữ liệu từ Serial (từ Uno) và gửi lên Blynk
  if (Serial.available() > 0) {
    serialData = Serial.readStringUntil('\n');
    serialData.trim();
    
    // ================================================
    // *** LOGIC MOI DE GUI THONG BAO ***
    // ================================================
    if (serialData.startsWith("notify:")) {
      // Nếu Arduino gửi lệnh "notify:", chúng ta sẽ gửi thông báo
      // Ví dụ: "notify:Cua vua duoc mo!"
      
      // Lấy phần tin nhắn (bỏ 7 ký tự "notify:")
      String notifyMsg = serialData.substring(7);
      
      // Gửi thông báo
      // [DA SUA LOI] Dùng 'logEvent' cho Blynk 2.0
      // Blynk.notify(notifyMsg); // <- Lỗi (Blynk 1.0)
      Blynk.logEvent("notify", notifyMsg); // <- Sửa thành hàm này.
      
      // Đồng thời cũng hiển thị lên V4
      Blynk.virtualWrite(V4, notifyMsg);
      
    } else {
      // Nếu là tin nhắn bình thường, chỉ hiển thị lên V4
      Blynk.virtualWrite(V4, serialData);
    }
  }
}