#include <WiFi.h>
#include <WiFiUdp.h>
#include "VideoStream.h"
#include "AmebaFatFS.h"
#include "GenAI.h"
#include "rtc.h"
#include <ArduinoJson.h>
#include <time.h>

// WiFi 設定
char ssid[] = "Galaxy A52s 5G"; // 您的 WiFi SSID
char pass[] = "dhxq7992";       // 您的 WiFi 密碼
String Gemini_key = "AIzaSyAftGIhpKV2EfulUYAfAd2WlIWTWgcaPBI"; // 請確認有效的 Gemini API 金鑰

// 檔案系統與 TTS 設定
AmebaFatFS fs;
GenAI llm;
GenAI tts;
String timeMp3Filename = "time_speech.mp3";    // 時間語音檔案
String sceneMp3Filename = "scene_description.mp3"; // 場景描述語音檔案

// 觸摸輸入設定 (時間查詢)
int timeTouchPin = A2;         // A2 用於時間查詢觸摸感測
int timeTouchThreshold = 7;    // 時間查詢觸摸閾值
const int sampleCount = 3;     // 取樣次數
const int sampleDelay = 5;     // 取樣延遲（毫秒）

// 觸摸感測器設定 (場景描述)
const int sceneTouchPin = A0;  // A0 用於場景描述觸摸感測
const int sceneTouchThreshold = 500; // 場景描述觸摸閾值

// 相機設定
VideoSetting config(768, 768, CAM_FPS, VIDEO_JPEG, 1);
#define CHANNEL 0
uint32_t img_addr = 0;
uint32_t img_len = 0;

// RTC 設定
long long seconds = 0;
struct tm *timeinfo;

// WiFi 客戶端
WiFiSSLClient client;

// 初始化 WiFi
bool initWiFi() {
  for (int i = 0; i < 3; i++) {
    WiFi.begin(ssid, pass);
    Serial.print("連接到 ");
    Serial.println(ssid);

    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((millis() - startTime) > 5000) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi 已連線，IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.println("WiFi 連線失敗，重試中...");
    delay(1000);
  }
  Serial.println("WiFi 連線失敗。");
  return false;
}

// 播放 MP3
void sdPlayMP3(String filename) {
  fs.begin();
  String filepath = String(fs.getRootPath()) + filename;
  File file = fs.open(filepath, MP3);
  if (file) {
    Serial.println("正在播放 MP3: " + filepath);
    file.setMp3DigitalVol(120);
    file.playMp3();
    file.close();
    Serial.println("MP3 播放完成。");
  } else {
    Serial.println("無法開啟 MP3 檔案: " + filepath);
  }
  fs.end();
}

// 獲取時間字串
String getTimeString() {
  seconds = rtc.Read();
  if (seconds <= 0) {
    Serial.println("RTC 讀取失敗，時間無效。");
    return "";
  }
  timeinfo = localtime(&seconds);
  char timeStr[20];
  snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(timeStr);
}

// 穩定讀取 ADC，並記錄最大值 (用於時間查詢)
int readStableTouchValue(int pin, int &maxValue) {
  long sum = 0;
  maxValue = 0;
  for (int i = 0; i < sampleCount; i++) {
    int value = analogRead(pin);
    sum += value;
    if (value > maxValue) maxValue = value;
    delay(sampleDelay);
  }
  return sum / sampleCount;
}

// 動態校準時間觸摸閾值
void calibrateTimeThreshold() {
  Serial.println("校準時間觸摸閾值中（請勿觸摸 A2 感應器）...");
  int baseline = 0;
  int dummyMax;
  for (int i = 0; i < 10; i++) {
    baseline += readStableTouchValue(timeTouchPin, dummyMax);
    delay(100);
  }
  baseline /= 10;
  timeTouchThreshold = max(baseline + 5, 7); // 確保閾值至少為 7
  Serial.print("時間觸摸校準完成，基線值: ");
  Serial.print(baseline);
  Serial.print("，閾值: ");
  Serial.println(timeTouchThreshold);
}

// 初始化 RTC
void initRTC() {
  rtc.Init();
  long long epochTime = rtc.SetEpoch(2025, 6, 12, 13, 54, 0); // 設定初始時間
  rtc.Write(epochTime);
  Serial.println("RTC 已初始化。");
}

void setup() {
  Serial.begin(115200);

  // 初始化腳位
  pinMode(timeTouchPin, INPUT);
  pinMode(sceneTouchPin, INPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_G, OUTPUT);

  // 校準時間觸摸閾值
  calibrateTimeThreshold();

  // 初始化 RTC
  initRTC();

  // 初始化檔案系統
  fs.begin();

  // 初始化 WiFi
  if (!initWiFi()) {
    Serial.println("WiFi 未連線，API 功能可能失效。");
  }

  // 初始化相機
  config.setRotation(0);
  Camera.configVideoChannel(CHANNEL, config);
  Camera.videoInit();
  Camera.channelBegin(CHANNEL);
  Camera.printInfo();

  // 除錯：檢查初始狀態
  int maxValue;
  Serial.print("初始時間觸摸 ADC 值 (A2): ");
  Serial.println(readStableTouchValue(timeTouchPin, maxValue));
  Serial.print("初始場景觸摸 ADC 值 (A0): ");
  Serial.println(analogRead(sceneTouchPin));
  String initTime = getTimeString();
  Serial.println("初始時間: " + (initTime.length() > 0 ? initTime : "無效"));
}

void loop() {
  // 檢查 WiFi 連線
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck >= 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi 已斷線，嘗試重新連線...");
      initWiFi();
    }
    lastWiFiCheck = millis();
  }

  // 時間查詢功能 (A2 觸摸)
  int timeMaxValue;
  int timeTouchValue = readStableTouchValue(timeTouchPin, timeMaxValue);
  static unsigned long lastTimeDebug = 0;
  if (millis() - lastTimeDebug >= 1000) {
    Serial.print("時間觸摸 ADC 值 (A2): ");
    Serial.print(timeTouchValue);
    Serial.print("，最大值: ");
    Serial.print(timeMaxValue);
    Serial.print("，閾值: ");
    Serial.println(timeTouchThreshold);
    if (timeTouchValue <= timeTouchThreshold) {
      Serial.println("時間觸摸未觸發");
    }
    lastTimeDebug = millis();
  }

  if (timeTouchValue > timeTouchThreshold) {
    Serial.println("檢測到時間觸摸！");

    // LED 反饋
    digitalWrite(LED_B, HIGH);
    digitalWrite(LED_G, HIGH);
    delay(500);
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_G, LOW);

    // 查詢時間
    String timeStr = getTimeString();
    if (timeStr.length() == 0) {
      Serial.println("時間字串無效，跳過 API 請求。");
      delay(1000);
      return;
    }
    Serial.println("當前時間: " + timeStr);

    // Gemini API 時間描述
    if (WiFi.status() == WL_CONNECTED) {
      String prompt = "請將以下時間轉為一句適合盲人理解的語音描述（繁體中文）： " + timeStr;
      Serial.println("正在發送 Gemini API 時間請求...");
      String text = llm.geminitext(Gemini_key, "gemini-1.5-flash", prompt, client);

      if (text.length() > 0) {
        Serial.println("時間描述: " + text);
        Serial.println("生成時間語音...");
        tts.googletts(timeMp3Filename, text, "zh-TW");
        delay(500);
        sdPlayMP3(timeMp3Filename);
      } else {
        Serial.println("Gemini API 未回應或返回空值。");
      }
    } else {
      Serial.println("WiFi 未連線，無法發送時間 API 請求。");
    }
    delay(1000);
  }

  // 場景描述功能 (A0 觸摸)
  int sceneTouchValue = analogRead(sceneTouchPin);
  static unsigned long lastSceneDebug = 0;
  if (millis() - lastSceneDebug >= 1000) {
    Serial.print("場景觸摸 ADC 值 (A0): ");
    Serial.print(sceneTouchValue);
    Serial.print("，閾值: ");
    Serial.println(sceneTouchThreshold);
    if (sceneTouchValue <= sceneTouchThreshold) {
      Serial.println("場景觸摸未觸發");
    }
    lastSceneDebug = millis();
  }

  if (sceneTouchValue > sceneTouchThreshold) {
    Serial.println("檢測到場景觸摸！");

    // 觸發指示燈
    for (int count = 0; count < 3; count++) {
      digitalWrite(LED_B, HIGH);
      delay(500);
      digitalWrite(LED_B, LOW);
      delay(500);
    }

    // 捕捉圖像
    Camera.getImage(CHANNEL, &img_addr, &img_len);

    // 使用 Gemini API 進行場景描述
    String prompt_msg = "請描述圖片中的場景，並用簡潔的中文說明。";
    String text = llm.geminivision(Gemini_key, "gemini-2.0-flash", prompt_msg, img_addr, img_len, client);

    // 加入時間戳
    String timeStamp = getTimeString();
    String fullText = "當前時間為 " + timeStamp + "，場景描述為：" + text;

    // 輸出到序列埠
    Serial.println(fullText);

    // 轉為語音並播放
    if (WiFi.status() == WL_CONNECTED) {
      tts.googletts(sceneMp3Filename, fullText, "zh-TW");
      delay(500);
      sdPlayMP3(sceneMp3Filename);
    } else {
      Serial.println("WiFi 未連線，無法發送場景 API 請求。");
    }

    // 防止重複觸發
    delay(2000);
  }

  delay(50);
}
