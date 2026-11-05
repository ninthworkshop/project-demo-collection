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
  Serial.println(F(" ESP32-C3-Zero + RC522 卡片讀取測試"));
  Serial.println(F("========================================"));
  Serial.println(F("請將寫入過資料的 Mifare 卡片靠近感應區...\n"));
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

  // 指定要讀取的區塊（與剛剛寫入相同的 Block 4）
  byte blockAddr = 4; 

  // 3. 設定 Mifare 金鑰（預設或寫入時使用的金鑰，通常為 6 個 0xFF）
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // 4. 進行金鑰驗證 (使用 A 金鑰)
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

  // 5. 讀取指定 Block 的資料（Mifare 每個 Block 固定為 16 bytes，需準備大小至少 18 的 buffer）
  byte buffer[18];
  byte size = sizeof(buffer);
  
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("[✖] 讀取資料失敗 (Read failed): "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  // 6. 成功讀取！在序列埠顯示內容
  Serial.print(F("[✔] Block 4 資料讀取成功!\n    ASCII 字串: "));
  for (byte i = 0; i < 16; i++) {
    // 過濾不可見字元，印出字串
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      Serial.print((char)buffer[i]);
    } else {
      Serial.print(F("."));
    }
  }
  Serial.println();

  // 同時印出 16 進位數值格式
  Serial.print(F("    Hex 格式:   "));
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] < 0x10) Serial.print(F("0"));
    Serial.print(buffer[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
  Serial.println(F("----------------------------------------"));

  // 7. 結束通訊
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(2000); // 讀取完成後暫停 2 秒
}
