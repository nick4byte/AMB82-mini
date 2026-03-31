#include "VideoStream.h"
#include "QRCodeScanner.h"
#include "WiFi.h"
#include <WiFiUdp.h>
#include "GenAI.h"
#include "AmebaFatFS.h"

#define CHANNEL 0
#define SCAN_INTERVAL 3000  // 掃描間隔3秒
#define PLAY_COOLDOWN 15000 // 播放後冷卻15秒

// WiFi 設定
char ssid[] = "Galaxy A52s 5G"; // 請替換為您的WiFi名稱
char pass[] = "dhxq7992";     // 請替換為您的WiFi密碼

// 初始化類物件
VideoSetting config(CHANNEL);
QRCodeScanner Scanner;
AmebaFatFS fs;
GenAI tts;

// MP3 檔案名稱
String mp3Filename = "location_speech.mp3";
String lastLocation = ""; // 記錄上一次掃描的地點

void initWiFi() {
    Serial.println("正在初始化WiFi...");
    for (int i = 0; i < 3; i++) {
        WiFi.begin(ssid, pass);
        delay(1000);
        Serial.print("正在連接到 ");
        Serial.println(ssid);

        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            if (millis() - startTime > 8000) {
                Serial.println("WiFi連線超時，重試...");
                break;
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi連線成功");
            Serial.print("STAIP 地址: ");
            Serial.println(WiFi.localIP());
            return;
        }
    }
    Serial.println("WiFi連線失敗，請檢查網路設定");
}

bool sdPlayMP3(String filename) {
    fs.begin();
    String filepath = String(fs.getRootPath()) + filename;
    File file = fs.open(filepath, MP3);
    
    if (!file) {
        Serial.println("無法開啟MP3檔案: " + filepath);
        fs.end();
        return false;
    }

    Serial.println("正在播放: " + filepath);
    file.setMp3DigitalVol(150); // 音量保持150
    file.playMp3();
    file.close();
    fs.end();
    
    Serial.println("MP3播放完成");
    return true;
}

void setup() {
    Serial.begin(115200);

    // 初始化攝影機
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Scanner.StartScanning();

    // 初始化WiFi
    initWiFi();
}

void loop() {
    // 檢查WiFi連線狀態
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi斷線，重新連線...");
        initWiFi();
        delay(SCAN_INTERVAL);
        return;
    }

    // 步驟1：掃描QR碼獲取地點名稱
    Scanner.GetResultString();
    if (Scanner.ResultString != nullptr) {
        String location = String(Scanner.ResultString); // 將char*轉為String
        if (location != lastLocation) {
            Serial.print("掃描到的地點名稱: ");
            Serial.println(location);

            // 步驟2：將地點名稱轉為語音MP3
            Serial.println("正在生成語音...");
            tts.googletts(mp3Filename, location, "zh-TW");
            delay(3000); // 增加延遲至3秒，確保MP3生成完成

            // 步驟3：播放MP3語音
            if (sdPlayMP3(mp3Filename)) {
                lastLocation = location; // 更新最後地點
                Serial.println("語音播放完成");
            } else {
                Serial.println("語音播放失敗");
            }

            // 冷卻時間，避免頻繁掃描和播放
            delay(PLAY_COOLDOWN);
        }
    }
    // 未掃描到新地點，正常掃描間隔
    delay(SCAN_INTERVAL);
}
