String Gemini_key = "AIzaSyCg15A8J9nVhZwF69XfwXccmGcFE8h52Dg"; // Your Gemini API key
char wifi_ssid[] = "Galaxy A52s 5G";   // Your network SSID
char wifi_pass[] = "dhxq7992";         // Your network password

#include <WiFi.h>
#include <WiFiUdp.h>
#include "GenAI.h"
#include "VideoStream.h"
#include "SPI.h"
#include "AmebaILI9341.h"
#include "TJpg_Decoder.h"
#include "AmebaFatFS.h"

WiFiSSLClient client;
GenAI llm;
AmebaFatFS fs;

VideoSetting config(768, 768, CAM_FPS, VIDEO_JPEG, 1);
#define CHANNEL 0

uint32_t img_addr = 0;
uint32_t img_len = 0;
const int buttonPin = 1;          // The number of the pushbutton pin

// Prompt for emotion detection and song recommendation in Chinese
String prompt_msg = "請偵測圖片中的人臉情緒，並從以下四首歌曲中嚴格選擇一首適合該情緒的歌曲名稱（僅回傳歌曲名稱，不含 .mp3 副檔名）：IBelieve（快樂）、LoversMisses（悲傷）、YUNGBLUD（憤怒）、Stumblin_In（驚訝）。若無人臉或情緒不明，選擇 IBelieve。歌曲必須是儲存在 SD 卡 mp3/ 資料夾中的 MP3 檔案。";

#define TFT_RESET 5
#define TFT_DC    4
#define TFT_CS    SPI_SS

AmebaILI9341 tft = AmebaILI9341(TFT_CS, TFT_DC, TFT_RESET);
#define ILI9341_SPI_FREQUENCY 20000000

// Valid song names for validation
const String validSongs[] = {"IBelieve", "LoversMisses", "YUNGBLUD", "Stumblin_In"};
const size_t VALID_SONGS_COUNT = 4;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  tft.drawBitmap(x, y, w, h, bitmap);
  return 1;
}

void initWiFi() {
  for (int i = 0; i < 2; i++) {
    WiFi.begin(wifi_ssid, wifi_pass);
    delay(1000);
    Serial.println("");
    Serial.print("連接到 ");
    Serial.println(wifi_ssid);

    uint32_t StartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime + 5000) < millis()) {
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("STAIP 位址: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
      break;
    }
  }
}

void init_tft() {
  tft.begin();
  tft.setRotation(2);
  tft.clr();
  tft.setCursor(0, 0);
  tft.setForeground(ILI9341_GREEN);
  tft.setFontSize(2);
}

void setup() {
  Serial.begin(115200);

  SPI.setDefaultFrequency(ILI9341_SPI_FREQUENCY);
  initWiFi();

  config.setRotation(0);
  Camera.configVideoChannel(CHANNEL, config);
  Camera.videoInit();
  Camera.channelBegin(CHANNEL);
  Camera.printInfo();
  
  pinMode(buttonPin, INPUT);
  pinMode(LED_B, OUTPUT);

  init_tft();
  tft.println("情緒音樂播放器");

  fs.begin(); // Initialize SD card once in setup
  TJpgDec.setJpgScale(2);
  TJpgDec.setCallback(tft_output);
}

void loop() {
  tft.setCursor(0, 30);
  tft.println("按下按鈕捕捉圖片");
  if (digitalRead(buttonPin) == 1) {
    tft.clr();
    tft.setCursor(0, 0);
    tft.println("正在捕捉圖片");

    // Blink LED to indicate capture
    for (int count = 0; count < 3; count++) {
      digitalWrite(LED_B, HIGH);
      delay(500);
      digitalWrite(LED_B, LOW);
      delay(500);
    }

    // Capture image
    Camera.getImage(0, &img_addr, &img_len);

    // Display image
    TJpgDec.getJpgSize(0, 0, (uint8_t *)img_addr, img_len);
    TJpgDec.drawJpg(0, 0, (uint8_t *)img_addr, img_len);

    // Detect emotion and get song recommendation using Gemini API
    String songName = llm.geminivision(Gemini_key, "gemini-2.0-flash", prompt_msg, img_addr, img_len, client);
    Serial.println("API 回傳歌曲: " + songName);

    // Validate songName
    bool isValidSong = false;
    for (size_t i = 0; i < VALID_SONGS_COUNT; i++) {
      if (songName == validSongs[i]) {
        isValidSong = true;
        break;
      }
    }
    if (!isValidSong) {
      Serial.println("無效歌曲名稱，回退到 IBelieve");
      tft.println("無效歌曲，回退到 IBelieve");
      songName = "IBelieve";
    }

    Serial.println("推薦歌曲: " + songName);
    tft.setCursor(0, 30);
    tft.println("推薦歌曲: " + songName);

    // Play the recommended song
    String mp3Filename = "mp3/" + songName + ".mp3";
    tft.println("播放: " + mp3Filename);
    sdPlayMP3(mp3Filename);
  }
}

void sdPlayMP3(String filename) {
  fs.begin();
  String filepath = String(fs.getRootPath()) + filename;
  File file = fs.open(filepath, MP3);
  if (file) {
    file.setMp3DigitalVol(175);
    file.playMp3();
    file.close();
  } else {
    Serial.println("無法開啟 MP3 檔案: " + filepath);
    tft.println("歌曲未找到，播放預設歌曲");
    String defaultMp3 = "mp3/IBelieve.mp3";
    File defaultFile = fs.open(String(fs.getRootPath()) + defaultMp3, MP3);
    if (defaultFile) {
      defaultFile.setMp3DigitalVol(175);
      defaultFile.playMp3();
      defaultFile.close();
    } else {
      Serial.println("無法開啟預設 MP3 檔案: " + defaultMp3);
      tft.println("預設歌曲未找到");
    }
  }
  fs.end();
}
