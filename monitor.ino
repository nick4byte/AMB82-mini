#include <WiFi.h>
#include <AmebaFatFS.h>
#include "VideoStream.h"
#include "GenAI.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define CHANNEL 0
VideoSetting config(768, 768, CAM_FPS, VIDEO_JPEG, 1);

char wifi_ssid[] = "Wowo";    // Your network SSID
char wifi_pass[] = "my89371546";        // Your network password
String Gemini_key = "AIzaSyBzpwqZ3fKDAVSJAkgGkaiUFgH1Ais0WuA";                // Paste your Gemini API key here
String prompt_msg = "Please describe the image, and if there is text, please summarize the content";

uint32_t img_addr = 0;
uint32_t img_len = 0;

AmebaFatFS fs;
WiFiSSLClient client;
GenAI llm;

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000); // UTC+8 for Taiwan, update every 60 seconds

String previousText = "";  // Store previous Gemini response
unsigned long lastCaptureTime = 0;
const unsigned long captureInterval = 60000; // 1 minute in milliseconds

void initWiFi() {
    for (int i = 0; i < 2; i++) {
        WiFi.begin(wifi_ssid, wifi_pass);
        Serial.print("Connecting to ");
        Serial.println(wifi_ssid);

        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            if (millis() - startTime > 5000) break;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi");
            Serial.println(WiFi.localIP());
            break;
        }
    }
}

String getDateTimeString() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);

    char datetime[20];
    snprintf(datetime, sizeof(datetime), "%04d%02d%02d_%02d%02d%02d",
             ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
             ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    return String(datetime);
}

void setup() {
    Serial.begin(115200);

    // Initialize WiFi
    initWiFi();

    // Initialize NTP Client
    timeClient.begin();
    timeClient.update();
    Serial.println("NTP Time: " + timeClient.getFormattedTime());

    // Initialize Camera
    config.setRotation(0);
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);
    Camera.printInfo();

    // Initialize SD Card
    fs.begin();

    // Initialize LED pins
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
}

void loop() {
    if (millis() - lastCaptureTime >= captureInterval) {
        // Blink LED to indicate capture
        digitalWrite(LED_B, HIGH);
        delay(200);
        digitalWrite(LED_B, LOW);

        // Capture image
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        Serial.println("Image captured");

        // Send to Gemini Vision
        String currentText = llm.geminivision(Gemini_key, "gemini-2.0-flash", prompt_msg, img_addr, img_len, client);
        Serial.println("Gemini Response: " + currentText);

        // Compare with previous response
        if (currentText != previousText && currentText != "") {
            String timestamp = getDateTimeString();
           
            // Save image
            String imgFilename = String(fs.getRootPath()) + timestamp + ".jpg";
            File imgFile = fs.open(imgFilename);
            imgFile.write((uint8_t *)img_addr, img_len);
            imgFile.close();
            Serial.println("Image saved: " + imgFilename);

            // Save text
            String txtFilename = String(fs.getRootPath()) + timestamp + ".txt";
            File txtFile = fs.open(txtFilename);
            txtFile.print(currentText);
            txtFile.close();
            Serial.println("Text saved: " + txtFilename);

            // Update previous text
            previousText = currentText;

            digitalWrite(LED_G, HIGH);
            delay(200);
            digitalWrite(LED_G, LOW);
        } else {
            Serial.println("No change in scene, skipping save");
        }

        // Update last capture time
        lastCaptureTime = millis();
    }

}
Explaination:
