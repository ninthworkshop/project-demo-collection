#include <Wire.h>
#include <Adafruit_PN532.h>

// ESP32 DEVKIT1 標準 I2C 腳位
#define I2C_SDA 21
#define I2C_SCL 22

#define PN532_IRQ   -1
#define PN532_RESET -1

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== ESP32 + PN532 MIFARE Data Reader ===");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // 100kHz 穩定頻率

  nfc.begin();

  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("錯誤：找不到 PN532 晶片！");
    while (1);
  }

  Serial.print("成功偵測到晶片！韌體版本: PN5");
  Serial.println((version >> 24) & 0xFF, HEX);
  
  if (!nfc.SAMConfig()) {
    Serial.println("錯誤：SAMConfig 設定失敗！");
    while (1);
  }

  Serial.println("初始化完成，請將卡片靠近天線...");
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  // 1. 偵測卡片
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500);

  if (success) {
    Serial.println("\n-----------------------------------------");
    Serial.println(">>> 發現卡片！");
    
    Serial.print("UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) Serial.print("0");
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // 確保是 4 位元組的 MIFARE Classic 卡片
    if (uidLength == 4) {
      // 定義要讀取的區塊（例如 Sector 1 的第一個資料區塊：Block 4）
      uint8_t blocknum = 4; 
      
      // MIFARE 預設密碼（通常為 6 個 0xFF）
      uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

      // 2. 進行密碼 A 驗證
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blocknum, 0, keyA);

      if (success) {
        Serial.print("區塊 ");
        Serial.print(blocknum);
        Serial.println(" 密碼驗證成功！");

        uint8_t data[16];
        // 3. 讀取該區塊的 16 Bytes 資料
        success = nfc.mifareclassic_ReadDataBlock(blocknum, data);

        if (success) {
          Serial.print("Block ");
          Serial.print(blocknum);
          Serial.print(" 資料 (Hex): ");
          for (uint8_t i = 0; i < 16; i++) {
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();

          // 順便印出 ASCII 文字對應（若有文字的話）
          Serial.print("Block ");
          Serial.print(blocknum);
          Serial.print(" 資料 (ASCII): ");
          for (uint8_t i = 0; i < 16; i++) {
            if (data[i] >= 32 && data[i] <= 126) {
              Serial.print((char)data[i]);
            } else {
              Serial.print(".");
            }
          }
          Serial.println();
        } else {
          Serial.println("錯誤：讀取資料區塊失敗！");
        }
      } else {
        Serial.println("錯誤：密碼驗證失敗！(可能該區塊已被更改密碼或非標準卡片)");
      }
    } else {
      Serial.println("此卡片不是 4-byte UID 的 MIFARE Classic 卡，跳過區塊讀取。");
    }
    
    Serial.println("-----------------------------------------");
    delay(1500); // 避免連續讀取洗版
  }
}