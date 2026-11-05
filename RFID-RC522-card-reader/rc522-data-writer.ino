#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN   3   // RC522 RST 腳位
#define SS_PIN    7   // RC522 SDA (SS) 腳位

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  // 初始化 SPI 匯流排 (SCK=4, MISO=5, MOSI=6, SS=7)
  SPI.begin(4, 5, 6, 7);
  mfrc522.PCD_Init();
  
  Serial.println(F("\n========================================"));
  Serial.println(F(" ESP32-C3-Zero + RC522 卡片寫入測試"));
  Serial.println(F("========================================"));
  Serial.println(F("請將 Mifare 卡片靠近感應區以進行寫入...\n"));
}

void loop() {
  // 1. 確認是否有新卡片靠近
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // 2. 讀取卡片 UID
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("[✔] 偵測到卡片 UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // 3. 準備要寫入的資料（必須剛好是 16 個位元組 bytes）
  // 這裡我們寫入字串 "Hello RFID 2026!"
  byte dataBlock[16] = {
    'H', 'e', 'l', 'l', 'o', ' ', 'R', 'F', 'I', 'D', ' ', '2', '0', '2', '6', '!'
  };

  // 指定要寫入的區塊（Sector 1 的第 0 個區塊，也就是 Block 4）
  // 注意：絕對不要寫入每個 Sector 的最後一個塊（如 Block 3, 7, 11...），那是金鑰區，寫錯會導致該區塊鎖死！
  byte blockAddr = 4; 

  // 4. 設定 Mifare 金鑰（預設新卡片金鑰通常皆為 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF）
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // 5. 進行金鑰驗證 (使用 A 金鑰)
  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
    blockAddr, 
    &key, 
    &(mfrc522.uid)
  );
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[✖] 金鑰驗證失敗 (Authentication failed): "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  Serial.println(F("[i] 金鑰驗證成功，準備寫入資料..."));

  // 6. 執行寫入資料到指定的 Block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[✖] 寫入資料失敗 (Write failed): "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  
  Serial.println(F("[✔] 資料成功寫入卡片 Block 4！"));
  Serial.println(F("----------------------------------------"));

  // 7. 結束通訊
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(2000); // 寫入完成後暫停 2 秒
}
