/**
 * @file  inet-radio-streamer.ino
 * @author Joseph (Makero Lab)
 * @brief High-performance Internet Radio Streamer using ESP32-S3 and MAX98357A.
 * * --- PREREQUISITES / BEFORE YOU USE ---
 * 1. Hardware: 
 * - ESP32-S3 DevKitC-1 (or compatible S3 module)
 * - MAX98357A I2S DAC Module
 * - 4-8 Ohm Speaker (Side-firing cavity speaker recommended)
 * 2. Libraries: 
 * - ESP8266Audio (Install via Arduino Library Manager)
 * 3. Wiring:
 * - BCLK -> GPIO 5
 * - LRC  -> GPIO 4
 * - DIN  -> GPIO 6
 * - VIN  -> 5V / GND -> GND
 * 4. WiFi: Update 'ssid' and 'password' variables before uploading.
 */

#include <WiFi.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// --- Configuration Constants ---
#define I2S_BCLK      5
#define I2S_LRC       4
#define I2S_DOUT      6

// Network Credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Buffer Settings: 128KB for smooth streaming stability
const int BUFFER_SIZE = 131072; 
const int PREFILL_THRESHOLD = 65536; // 64KB target before playback starts

// Global Audio Objects
AudioGeneratorMP3         *mp3;
AudioFileSourceICYStream  *file;
AudioFileSourceBuffer     *buff;
AudioOutputI2S            *out;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n======================================");
    Serial.println("   MAKEROLAB - ESP32-S3 Audio Demo    ");
    Serial.println("======================================");

    // Set CPU to max performance for decoding
    setCpuFrequencyMhz(240);

    // Connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[OK] WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Configure I2S Output
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); 
    out->SetGain(0.8); // Adjust volume gain (0.0 to 1.0)

    // Initialize Stream Source (SomaFM Lush - 128kbps MP3)
    file = new AudioFileSourceICYStream("http://ice1.somafm.com/lush-128-mp3");
    buff = new AudioFileSourceBuffer(file, BUFFER_SIZE);
    mp3  = new AudioGeneratorMP3();

    // --- Deep Prefill Logic ---
    // This ensures enough data is cached to prevent stuttering during network spikes
    Serial.println("Buffering... Please wait.");
    uint32_t lastLevel = 0;
    unsigned long timeout = millis();
    
    while (buff->getFillLevel() < PREFILL_THRESHOLD) {
        // Exit if prefill takes longer than 15 seconds
        if (millis() - timeout > 15000) {
            Serial.println("\n[Warning] Prefill timeout. Attempting playback anyway.");
            break;
        }
        
        uint32_t currentLevel = buff->getFillLevel();
        if (currentLevel != lastLevel) {
            // Visual progress bar
            Serial.print("#");
            lastLevel = currentLevel;
        }
        delay(50); 
    }

    Serial.printf("\n[Buffer Ready] %u bytes cached.", (unsigned int)buff->getFillLevel());

    // Start Decoding
    if (mp3->begin(buff, out)) {
        Serial.println("\n>>> Playback Started!");
    } else {
        Serial.println("\n[Error] MP3 Generator failed to start.");
    }
}

void loop() {
    static unsigned long lastStatus = 0;

    if (mp3->isRunning()) {
        // The ESP32-S3 handles background tasks efficiently;
        // complex for-loops are generally unnecessary here.
        if (!mp3->loop()) {
            mp3->stop();
            Serial.println("\n[Stream Ended] Attempting to reconnect...");
            delay(1000);
        }
    } else {
        // Handle playback errors or stream stops
        Serial.println("MP3 decoding stopped.");
        delay(2000);
    }

    // Optional: Log buffer status every 5 seconds for debugging
    if (millis() - lastStatus > 5000) {
        // Serial.printf("Buffer Level: %u\n", (unsigned int)buff->getFillLevel());
        lastStatus = millis();
    }
}
