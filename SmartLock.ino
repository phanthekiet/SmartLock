#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// ========== CẤU HÌNH CHÂN ==========
#define SS_PIN 10
#define RST_PIN 9
#define LED_PIN 2
#define RELAY_PIN 3
#define BUZZER_PIN A0
#define I2C_ADDR 0x20
#define PASSWORD_ADDR 500

// ========== KHỞI TẠO THIẾT BỊ ==========
SoftwareSerial mySerial(4, 5);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
I2CKeyPad keyPad(I2C_ADDR);

// ========== KEYPAD LAYOUT ==========
const char keys[4][4] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// ========== BIẾN TRẠNG THÁI ==========
String password = "1234";

struct SystemState {
  String input = "";
  String newPassword = "";
  bool addCardMode = false;
  bool addFingerMode = false;
  bool deleteCardMode = false;
  bool deleteFingerMode = false;
  bool changePasswordMode = false;
  bool systemPermanentlyLocked = false;
  unsigned long lastAction = 0;
  uint8_t wrongAttempts = 0;
  static const unsigned long IDLE_TIMEOUT = 5000;
  static const uint8_t MAX_ATTEMPTS = 3;
} state;

// ========== SETUP ==========
void setup() {
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
  SPI.begin();
  initKeypad();
  initRFID();
  initFingerprint();
  initLCD();
  loadPassword();
  showIdle();
  Serial.println(F("=== He thong khoa thong minh ==="));
  Serial.println(F("A: Them the | B: Them van tay | C: Doi MK | D: Xoa the"));
  Serial.println(F("# (Luc ranh): Xoa Van Tay"));
}

// ========== KHỞI TẠO THIẾT BỊ ==========
void initKeypad() {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0xFF);
  Wire.endTransmission();
  delay(100);
}
void initRFID() {
  mfrc522.PCD_Init();
  delay(100);
}
void initFingerprint() {
  finger.begin(57600);
  delay(100);
  if (finger.verifyPassword()) {
    Serial.println(F("Cam bien van tay OK!"));
    finger.getParameters();
    Serial.print(F("Capacity: "));
    Serial.println(finger.capacity);
  } else {
    Serial.println(F("Loi cam bien van tay!"));
  }
}
void initLCD() {
  lcd.init();
  lcd.backlight();
}

// ========== LOOP CHÍNH ==========
// [SỬA LỖI] Thêm check Serial ngay cả khi bị khóa
void loop() {
  // [SỬA] ƯU TIÊN CAO NHẤT: Kiểm tra khóa vĩnh viễn
  if (state.systemPermanentlyLocked) {
    digitalWrite(BUZZER_PIN, (millis() / 500) % 2); // Buzzer nhấp nháy
    checkFingerprint(); // Chỉ chạy hàm kiểm tra vân tay (để mở khóa)

    // [THÊM MỚI] Vẫn lắng nghe lệnh từ ESP ngay cả khi bị khóa
    char cmd = Serial.available() ? toupper(Serial.read()) : 0;
    if (cmd == 'U') { // 'U' = Unlock (Mở khóa)
      state.systemPermanentlyLocked = false;
      state.wrongAttempts = 0;
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi
      Serial.println(F("!!! HE THONG DA MO KHOA (REMOTE) !!!"));
      
      // *** THÊM THÔNG BÁO CHO ESP ***
      Serial.println(F("notify:Hệ thống đã được MỞ KHÓA từ xa!"));

      lcd.clear();
      lcd.print(F("Da mo khoa HE"));
      delay(1500); 
      showIdle(); 
    }
    
    delay(200);
    return; // Bỏ qua mọi thứ khác
  }

  // Nếu không bị khóa, chạy như bình thường
  char key = getKeyPressed();
  char cmd = Serial.available() ? toupper(Serial.read()) : 0;

  // Cấu trúc "if... else if... else" để sửa lỗi lặp phím
  if (state.changePasswordMode) {
    if (key) handleChangePassword(key);
  } else if (state.addCardMode) {
    addCardRoutine();
  } else if (state.addFingerMode) {
    addFingerRoutine();
  } else if (state.deleteCardMode) {
    deleteCardRoutine();
  } else if (state.deleteFingerMode) {
    if (key) handleDeleteFinger(key);
  } else {
    // CHỈ chạy các hàm này NẾU KHÔNG ở chế độ đặc biệt
    if (key) {
      if (handleKey(key)) return;
    }
    if (cmd) {
      if (handleSerial(cmd)) return; // Xử lý lệnh từ ESP (A, B, C, D, O)
    }
    checkFingerprint();
    checkRFID();
    // Timeout 5s chỉ áp dụng khi ở màn hình chính/nhập mật khẩu
    if (millis() - state.lastAction > state.IDLE_TIMEOUT) {
      if (state.input.length() > 0) {
        state.input = "";
        showIdle();
      } else {
        state.lastAction = millis();
      }
    }
  }
  delay(200);
}

// ========== ĐỌC PHÍM ==========
char getKeyPressed() {
  uint8_t key = keyPad.getKey();

  if (key < 16) {
    uint8_t row = key % 4;
    uint8_t col = key / 4;
    return keys[row][col];
  }

  return 0;
}

// ========== XỬ LÝ PHÍM BẤM ==========
bool handleKey(char key) {
  state.lastAction = millis();
  Serial.print(F("Phim: "));
  Serial.println(key);
  switch (key) {
    case '*':
      checkPassword();
      break;
    case '#':
      if (state.input.length() > 0) {
        state.input.remove(state.input.length() - 1);
        showInput();
      } else {
        startDeleteFinger();
        return true;
      }
      break;
    case 'A':
      startAddCard();
      return true;
    case 'B':
      startAddFinger();
      return true;
    case 'C':
      startChangePassword();
      return true;
    case 'D':
      startDeleteCard();
      return true;
    default:
      if (key >= '0' && key <= '9') {
        if (state.input.length() == 0) {
          lcd.clear();
          lcd.print(F("Nhap mat khau:"));
          lcd.setCursor(0, 1);
        }
        state.input += key;
        showInput();
      }
  }
  return false;
}

// ========== HIỂN THỊ ==========
void showIdle() {
  lcd.clear();
  lcd.print(state.systemPermanentlyLocked ? F("HE THONG KHOA!") : F("He thong khoa"));
  state.lastAction = millis();
}
void showInput() {
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < state.input.length(); i++) {
    lcd.print('*');
  }
}

// ========== KIỂM TRA MẬT KHẨU ==========
void checkPassword() {
  Serial.print(F("MK: "));
  Serial.println(state.input);
  Serial.println(password);
  if (state.input == password) {
    lcd.clear();
    lcd.print(F("MK dung!"));
    Serial.println(F("-> Dung"));
    state.wrongAttempts = 0;
    delay(500);
    openDoor();
  } else {
    state.wrongAttempts++;
    lcd.clear();
    lcd.print(F("MK sai!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Con "));
    lcd.print(state.MAX_ATTEMPTS - state.wrongAttempts);
    lcd.print(F(" lan"));
    Serial.print(F("-> Sai ("));
    Serial.print(state.wrongAttempts);
    Serial.println(F("/3)"));
    if (state.wrongAttempts >= state.MAX_ATTEMPTS) {
      lockSystemPermanently();
    } else {
      delay(1500);
      showIdle();
    }
  }
  state.input = "";
}

// ========== KHÓA HỆ THỐNG ==========
void lockSystemPermanently() {
  state.systemPermanentlyLocked = true;
  lcd.clear();
  lcd.print(F("HE THONG KHOA!"));
  lcd.setCursor(0, 1);
  lcd.print(F("Dung van tay..."));
  digitalWrite(BUZZER_PIN, HIGH); 
  Serial.println(F("!!! KHOA VV - DUNG VAN TAY DE MO !!!"));
  
  // ================================================
  // *** THÊM THÔNG BÁO CẢNH BÁO QUAN TRỌNG ***
  // ================================================
  Serial.println(F("notify:!!! CANH BAO: Khoa bi vo hieu hoa do nhap sai 3 lan!"));
}

// ========== ĐỔI MẬT KHẨU ==========
void startChangePassword() {
  state.changePasswordMode = true;
  state.input = "";
  state.newPassword = "";
  lcd.clear();
  lcd.print(F("Nhap MK cu:"));
  lcd.setCursor(0, 1);
  lcd.print(F("(D=Huy)          ")); 
  Serial.println(F("Che do doi MK (Nhan D de Huy)"));
}
void handleChangePassword(char key) {
  state.lastAction = millis(); 
  if (key == '*') {
    if (state.newPassword == "") {
      verifyOldPassword();
    } else if (state.newPassword == "VERIFIED") {
      setNewPassword();
    } else {
      confirmNewPassword();
    }
  } else if (key == '#') {
    if (state.input.length() > 0) {
      state.input.remove(state.input.length() - 1);
      showInput();
    }
  } else if (key == 'D') { 
    cancelChangePassword();
  } else {
    if (key >= '0' && key <= '9') {
        state.input += key;
        Serial.println(state.input);
        showInput();
    }
  }
}
void verifyOldPassword() {
  if (state.input == password) {
    lcd.clear();
    lcd.print(F("Nhap MK moi:"));
    lcd.setCursor(0, 1);
    lcd.print(F("(D=Huy)          ")); 
    state.newPassword = "VERIFIED";
    state.input = "";
  } else {
    lcd.clear();
    lcd.print(F("MK cu sai!"));
    delay(1500);
    cancelChangePassword();
  }
}
void setNewPassword() {
  if (state.input.length() >= 3) {
    state.newPassword = state.input;
    state.input = "";
    lcd.clear();
    lcd.print(F("Nhap lai MK:"));
    lcd.setCursor(0, 1);
    lcd.print(F("(D=Huy)          ")); 
  } else {
    lcd.clear();
    lcd.print(F("MK qua ngan!"));
    delay(1000);
    lcd.clear();
    lcd.print(F("Nhap MK moi:"));
    lcd.setCursor(0, 1);
    lcd.print(F("(D=Huy)          "));
    state.input = "";
  }
}
void confirmNewPassword() {
  if (state.input == state.newPassword) { 
    password = state.newPassword;
    savePassword();
    lcd.clear();
    lcd.print(F("Doi MK thanh cong"));
    Serial.print(F("MK moi: "));
    Serial.println(password);
    delay(2000);
  } else {
    lcd.clear();
    lcd.print(F("MK khong khop!"));
    delay(1500);
  }
  cancelChangePassword();
}
void cancelChangePassword() {
  state.changePasswordMode = false;
  state.input = "";
  state.newPassword = "";
  Serial.println(F("Thoat che do Doi MK."));
  showIdle();
}

// ========== EEPROM ==========
void savePassword() {
  for (uint8_t i = 0; i < password.length(); i++) {
    EEPROM.write(PASSWORD_ADDR + i, password[i]);
  }
  EEPROM.write(PASSWORD_ADDR + password.length(), 0);
  Serial.println(F("Da luu MK"));
}
void loadPassword() {
  String loadedPW = "";
  for (uint8_t i = 0; i < 20; i++) {
    char c = EEPROM.read(PASSWORD_ADDR + i);
    if (c == 0 || c == 0xFF) break;
    loadedPW += c;
  }
  if (loadedPW.length() >= 4) {
    password = loadedPW;
    Serial.print(F("Tai MK: "));
    Serial.println(password);
  } else {
    Serial.println(F("MK mac dinh: 1234"));
  }
}

// ========== MỞ CỬA ==========
void openDoor() {
  lcd.clear();
  lcd.print(F("Mo cua..."));
  Serial.println(F("Cua mo")); // Gửi lên V4
  
  // ================================================
  // *** GỬI THÔNG BÁO CHO ESP8266 TẠI ĐÂY ***
  // ================================================
  Serial.println(F("notify:Cua vua duoc mo!"));

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
  delay(4000);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  lcd.clear();
  lcd.print(F("Khoa lai"));
  Serial.println(F("Cua khoa")); // Gửi lên V4
  delay(1000);
  showIdle();
}

// ========== SERIAL COMMAND ==========
bool handleSerial(char cmd) {
  switch (cmd) {
    case 'A': startAddCard(); return true;
    case 'B': startAddFinger(); return true;
    case 'C': startChangePassword(); return true;
    case 'D': startDeleteCard(); return true; 
    case 'O': openDoor(); return true;
    // Lệnh 'U' được xử lý riêng trong loop()
    default: 
      Serial.print(F("Lenh sai: ")); 
      Serial.println(cmd);
  }
  return false;
}

// ========== RFID ==========
void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  lcd.clear();
  lcd.print(F("Dang quet the..."));
  String id = getCardID();
  Serial.print(F("RFID: "));
  Serial.println(id);
  if (checkCard(id)) {
    Serial.println(F("-> Hop le"));
    lcd.clear();
    lcd.print(F("The hop le!"));
    delay(800);
    state.wrongAttempts = 0;
    openDoor();
  } else {
    // [SỬA] Thêm logic đếm lỗi cho thẻ RFID
    state.wrongAttempts++;
    lcd.clear();
    lcd.print(F("The sai!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Con "));
    lcd.print(state.MAX_ATTEMPTS - state.wrongAttempts);
    lcd.print(F(" lan"));
    Serial.print(F("-> Sai ("));
    Serial.print(state.wrongAttempts);
    Serial.println(F("/3)"));
    if (state.wrongAttempts >= state.MAX_ATTEMPTS) {
      lockSystemPermanently(); // Gọi hàm khóa vĩnh viễn
    } else {
      delay(1500);
      showIdle();
    }
  }
}
String getCardID() {
  String id = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    id += String(mfrc522.uid.uidByte[i], HEX);
  }
  id.toUpperCase();
  mfrc522.PICC_HaltA();
  return id;
}
bool checkCard(String id) {
  for (int i = 0; i < PASSWORD_ADDR; i += 20) {
    String storedID = readEEPROMString(i, 20);
    if (storedID == id) return true;
  }
  return false;
}
void startAddCard() {
  state.addCardMode = true;
  lcd.clear();
  lcd.print(F("Them the moi..."));
  delay(1000);
  lcd.clear();
  lcd.print(F("Quet the..."));
  Serial.println(F("Che do them the"));
  Serial.println(F("notify:Bat dau them THE TU...")); // Thông báo
}
void addCardRoutine() {
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String id = getCardID();
      Serial.print(F("The moi: "));
      Serial.println(id);
      if (checkCard(id)) {
        lcd.clear();
        lcd.print(F("The da ton tai!"));
        Serial.println(F("-> The nay da co, khong them"));
        Serial.println(F("notify:Them the THAT BAI (The da ton tai)")); // Thông báo
        delay(2000);
        state.addCardMode = false;
        showIdle();
        return; 
      }
      lcd.clear();
      lcd.print(F("Luu the..."));
      int addr = findEmptySlot();
      writeEEPROMString(addr, id);
      lcd.clear();
      lcd.print(F("Luu thanh cong!"));
      Serial.println(F("Luu OK!"));
      Serial.println(F("notify:Da them THE TU moi thanh cong!")); // Thông báo
      delay(1500);
      state.addCardMode = false;
      showIdle();
      return;
    }
    delay(50);
  }
  lcd.clear();
  lcd.print(F("Quet that bai!"));
  Serial.println(F("Khong phat hien"));
  Serial.println(F("notify:Them the THAT BAI (timeout)")); // Thông báo
  delay(1500);
  state.addCardMode = false;
  showIdle();
}
int findEmptySlot() {
  for (int i = 0; i < PASSWORD_ADDR; i += 20) {
    if (EEPROM.read(i) == 0xFF) return i;
  }
  return 0;
}

// ========== XÓA THẺ RFID ==========
void startDeleteCard() {
  state.deleteCardMode = true;
  lcd.clear();
  lcd.print(F("Che do xoa the"));
  lcd.setCursor(0, 1);
  lcd.print(F("Quet the de xoa"));
  Serial.println(F("Che do xoa the: Quet de xoa..."));
  Serial.println(F("notify:Bat dau che do XOA THE...")); // Thông báo
}
void deleteCardRoutine() {
  unsigned long start = millis();
  while (millis() - start < 5000) { 
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String id = getCardID();
      Serial.print(F("Quet de xoa: "));
      Serial.println(id);
      bool cardFound = false;
      for (int i = 0; i < PASSWORD_ADDR; i += 20) {
        String storedID = readEEPROMString(i, 20);
        if (storedID == id) {
          eraseEEPROMSlot(i); 
          cardFound = true;   
          Serial.print(F("-> Da xoa ban sao tai dia chi: "));
          Serial.println(i);
        }
      }
      if (cardFound) {
        lcd.clear();
        lcd.print(F("Da xoa (all)")); 
        Serial.println(F("-> Da xoa tat ca cac ban sao."));
        Serial.println(F("notify:Da XOA THE thanh cong!")); // Thông báo
        delay(2000);
      } else {
        lcd.clear();
        lcd.print(F("The khong ton tai"));
        Serial.println(F("-> The khong co trong he thong."));
        Serial.println(F("notify:Xoa the THAT BAI (Khong tim thay)")); // Thông báo
        delay(2000);
      }
      state.deleteCardMode = false;
      showIdle();
      return;
    }
    delay(50);
  }
  lcd.clear();
  lcd.print(F("Huy bo xoa the"));
  Serial.println(F("Huy bo (timeout)."));
  Serial.println(F("notify:Xoa the THAT BAI (timeout)")); // Thông báo
  delay(1500);
  state.deleteCardMode = false;
  showIdle();
}
void eraseEEPROMSlot(int addr) {
  for (int i = 0; i < 20; i++) {
    EEPROM.write(addr + i, 0xFF);
  }
}

// ========== VÂN TAY ==========
void checkFingerprint() {
  int id = getFingerprintID(); 
  if (id > 0) {
    Serial.print(F("Van tay ID: "));
    Serial.println(id);
    // [SỬA] Logic mở khóa vĩnh viễn bằng vân tay
    if (state.systemPermanentlyLocked) {
      state.systemPermanentlyLocked = false;
      state.wrongAttempts = 0;
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi
      Serial.println(F("!!! HE THONG DA MO KHOA (MASTER) !!!"));
      Serial.println(F("notify:Hệ thống đã MỞ KHÓA (bằng vân tay master)")); // Thông báo
      lcd.clear();
      lcd.print(F("Da mo khoa HE"));
      delay(1000); 
    }
    lcd.clear();
    lcd.print(F("Van tay hop le!"));
    delay(800);
    state.wrongAttempts = 0; 
    openDoor();
  }
}
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    return -1; 
  }
  p = finger.image2Tz(2); 
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  for (int i = 1; i <= finger.capacity; i++) {
    p = finger.loadModel(i);
    if (p != FINGERPRINT_OK) {
        continue; 
    }
    p = finger.createModel(); 
    if (p == FINGERPRINT_OK) {
      return i; 
    }
  }
  return -1;
}
void startAddFinger() {
  state.addFingerMode = true;
  lcd.clear();
  lcd.print(F("Them van tay..."));
  Serial.println(F("Che do them van tay"));
  Serial.println(F("notify:Bat dau them VAN TAY...")); // Thông báo
  delay(1000);
}
void addFingerRoutine() {
  int id = getNextFingerID();
  lcd.clear();
  lcd.print(F("ID: "));
  lcd.print(id);
  lcd.setCursor(0, 1);
  lcd.print(F("Dat ngon tay..."));
  Serial.print(F("ID: "));
  Serial.println(id);
  enrollFingerprint(id);
}
int getNextFingerID() {
  finger.getImage();
  finger.image2Tz(1); 
  for (int i = 1; i <= 127; i++) {
    uint8_t p = finger.loadModel(i);
    if (p == FINGERPRINT_OK) {
      continue;
    }
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      return i;
    }
    return i;
  }
  return 1; 
}
void enrollFingerprint(int id) {
  if (!waitAndCapture(1)) {
    Serial.println(F("notify:Them van tay THAT BAI!")); // Thông báo
    state.addFingerMode = false;
    return;
  }
  lcd.clear();
  lcd.print(F("Nha ngon tay ra"));
  Serial.println(F("Remove finger"));
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  lcd.clear();
  lcd.print(F("Dat lai lan 2"));
  Serial.print(F("ID ")); Serial.println(id);
  if (!waitAndCapture(2)) {
    Serial.println(F("notify:Them van tay THAT BAI!")); // Thông báo
    state.addFingerMode = false;
    return;
  }
  Serial.print(F("Creating model #")); Serial.println(id);
  uint8_t p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("Model failed"));
    if (p == FINGERPRINT_ENROLLMISMATCH) {
      lcd.clear();
      lcd.print(F("Van tay khong khop"));
      Serial.println(F("notify:Them THAT BAI (Van tay khong khop)")); // Thông báo
      delay(2000);
    }
    state.addFingerMode = false;
    return;
  }
  Serial.println(F("Prints matched!"));
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println(F("Stored!"));
    Serial.println(F("notify:Da them VAN TAY moi thanh cong!")); // Thông báo
    lcd.clear();
    lcd.print(F("Da luu van tay!"));
    delay(2000);
  } else {
    Serial.println(F("Store failed"));
    Serial.println(F("notify:Them THAT BAI (Loi bo nho)")); // Thông báo
  }
  state.addFingerMode = false;
  showIdle();
}
bool waitAndCapture(uint8_t slot) {
  while (finger.getImage() != FINGERPRINT_OK) {
    delay(50);
  }
  Serial.println(F("Image taken"));
  uint8_t p = finger.image2Tz(slot);
  if (p == FINGERPRINT_OK) {
    Serial.println(F("Image converted"));
    return true;
  }
  Serial.println(F("Convert failed"));
  return false;
}

// ========== XÓA VÂN TAY ==========
void startDeleteFinger() {
  state.deleteFingerMode = true;
  state.input = "";
  lcd.clear();
  lcd.print(F("Xoa Van Tay"));
  lcd.setCursor(0, 1);
  lcd.print(F("ID:? (#=OK, *=All)"));
  Serial.println(F("Che do xoa van tay. Nhap ID & #, hoac * de xoa het."));
  Serial.println(F("notify:Bat dau che do XOA VAN TAY...")); // Thông báo
}
void cancelDeleteFinger() {
  state.deleteFingerMode = false;
  state.input = "";
  Serial.println(F("Thoat che do xoa.")); 
  showIdle();
}
void handleDeleteFinger(char key) {
  state.lastAction = millis(); 
  if (key >= '0' && key <= '9') {
    state.input += key;
    lcd.setCursor(3, 1); 
    lcd.print(state.input);
  } 
  else if (key == '#') {
    if (state.input.length() > 0) {
      int id = state.input.toInt();
      deleteFingerprintByID(id); 
    }
  } 
  else if (key == '*') {
    deleteAllFingerprints(); 
  }
  else {
    cancelDeleteFinger();
  }
}
void deleteFingerprintByID(int id) {
  if (id > 0 && id <= 127) { 
    uint8_t p = finger.deleteModel(id); 
    if (p == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print(F("Da xoa ID: "));
      lcd.print(id);
      Serial.print(F("Da xoa van tay ID: "));
      Serial.println(id);
      Serial.println(F("notify:Da XOA VAN TAY thanh cong!")); // Thông báo
    } else {
      lcd.clear();
      lcd.print(F("Loi khi xoa ID: "));
      lcd.print(id);
      Serial.print(F("Loi xoa ID: "));
      Serial.println(id);
      Serial.println(F("notify:Xoa van tay THAT BAI (Loi)")); // Thông báo
    }
  } else {
    lcd.clear();
    lcd.print(F("ID khong hop le"));
    Serial.println(F("ID khong hop le."));
    Serial.println(F("notify:Xoa van tay THAT BAI (ID sai)")); // Thông báo
  }
  delay(2000);
  cancelDeleteFinger(); 
}
void deleteAllFingerprints() {
  lcd.clear();
  lcd.print(F("Dang xoa tat ca"));
  lcd.setCursor(0, 1);
  lcd.print(F("Xin cho..."));
  uint8_t p = finger.emptyDatabase(); 
  if (p == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print(F("Da xoa tat ca VT"));
      Serial.println(F("Da xoa toan bo van tay."));
      Serial.println(F("notify:Da XOA TOAN BO van tay!")); // Thông báo
  } else {
      lcd.clear();
      lcd.print(F("Xoa loi!"));
      Serial.println(F("Loi xoa toan bo CSDL."));
      Serial.println(F("notify:Xoa van tay THAT BAI (Loi CSDL)")); // Thông báo
  }
  delay(2000);
  cancelDeleteFinger(); 
}

// ========== HELPER EEPROM ==========
String readEEPROMString(int addr, int maxLen) {
  String str = "";
  for (int i = 0; i < maxLen; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0 || c == 0xFF) break;
    str += c;
  }
  return str;
}
void writeEEPROMString(int addr, String str) {
  for (uint8_t i = 0; i < str.length(); i++) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str.length(), 0);
}