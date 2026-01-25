String openAI_key = "";
String Gemini_key = "AIzaSyCrXM3c64lYIYO7C9Yu49vB33Uj9gqkFSk";
String Llama_key = "";
char wifi_ssid[] = "Galaxy A52s 5G";
char wifi_pass[] = "dhxq7992";

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
GenAI tts;

AmebaFatFS fs;
String mp3Filename = "test_play_google_tts.mp3";

VideoSetting config(768, 768, CAM_FPS, VIDEO_JPEG, 1);
#define CHANNEL 0

uint32_t img_addr = 0;
uint32_t img_len = 0;
const int buttonPin = 1;

#define TFT_RESET 5
#define TFT_DC 4
#define TFT_CS SPI_SS

AmebaILI9341 tft = AmebaILI9341(TFT_CS, TFT_DC, TFT_RESET);

#define ILI9341_SPI_FREQUENCY 20000000

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  tft.drawBitmap(x, y, w, h, bitmap);
  return 1;
}

void initWiFi() {
  for (int i = 0; i < 2; i++) {
    WiFi.begin(wifi_ssid, wifi_pass);
    delay(1000);
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    uint32_t StartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime + 5000) < millis()) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected, IP: ");
      Serial.println(WiFi.localIP());
      break;
    }
  }
}

void init_tft() {
  tft.begin();
  tft.setRotation(0);
  tft.clr();
  tft.setCursor(0, 0);
  tft.setForeground(ILI9341_GREEN);
  tft.setFontSize(2);
}

void setup() {
  Serial.begin(115200);
  SPI.setDefaultFrequency(ILI9341_SPI_FREQUENCY);
  initWiFi();

  config.setRotation(2);
  Camera.configVideoChannel(CHANNEL, config);
  Camera.videoInit();
  Camera.channelBegin(CHANNEL);
  Camera.printInfo();

  pinMode(buttonPin, INPUT);
  pinMode(LED_B, OUTPUT);

  init_tft();
  tft.println("GenAIVision_TTS_ReadWordCard");

  TJpgDec.setJpgScale(2);
  TJpgDec.setCallback(tft_output);

  tft.println("Press Button");
}

void blink_blue(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_B, HIGH);
    delay(500);
    digitalWrite(LED_B, LOW);
    delay(500);
  }
}

// 分割文字為每 20 字一段
String* splitStringByLength(String text, int length, int& numChunks) {
  numChunks = (text.length() + length - 1) / length;
  String* chunks = new String[numChunks];

  for (int i = 0; i < numChunks; i++) {
    int startIdx = i * length;
    int endIdx = min((i + 1) * length, (int)text.length()); // 修正此行
    chunks[i] = text.substring(startIdx, endIdx);
  }

  return chunks;
}

// 分段語音播放
void speakTextInChunks(String text, String lang) {
  int numChunks = 0;
  String* chunks = splitStringByLength(text, 20, numChunks);

  for (int i = 0; i < numChunks; i++) {
    tts.googletts(mp3Filename, chunks[i], lang);
    delay(500);
    sdPlayMP3(mp3Filename);
  }

  delete[] chunks;
}

void loop() {
  if (digitalRead(buttonPin) == 1) {
    tft.setCursor(0, 0);
    tft.println("Image captured!");
    blink_blue(2);

    Camera.getImage(0, &img_addr, &img_len);

    TJpgDec.getJpgSize(0, 0, (uint8_t *)img_addr, img_len);
    TJpgDec.drawJpg(0, 0, (uint8_t *)img_addr, img_len);

    String prompt_msg_img = "Just say the word in the picture?";
    String text = llm.geminivision(Gemini_key, "gemini-2.0-flash", prompt_msg_img, img_addr, img_len, client);
    tft.setCursor(0, 0);
    tft.println(text);

    speakTextInChunks(text, "en-US");

    String prompt_msg = "please make a short story with " + text;
    Serial.println(prompt_msg);
    String txt = llm.geminitext(Gemini_key, "gemini-2.0-flash", prompt_msg, client);
    tft.println(txt);

    speakTextInChunks(txt, "en-US");

    tft.println("Press Button");
  }
}

void sdPlayMP3(String filename) {
  fs.begin();
  String filepath = String(fs.getRootPath()) + filename;
  File file = fs.open(filepath, MP3);
  file.setMp3DigitalVol(128);
  file.playMp3();
  file.close();
  fs.end();
}
