#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define AUDIO_FILE        "/Audio.wav"

const char* ssid = "*****";         
const char* password = "*******";

#define pin_RECORD_BTN 34
#define LED 2

const char* responder_ip = "*******";  // IP of the responder board
int responder_port = 80;

bool I2S_Record_Init();
bool Record_Start(String filename);
bool Record_Available(String filename, float* audiolength_sec);

String SpeechToText_Deepgram(String filename);
void Deepgram_KeepAlive();
void sendCommandToResponder(String command);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);  // 10 times faster reaction after CR entered (default is 1000ms)

  pinMode(LED, OUTPUT);
  pinMode(pin_RECORD_BTN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to wifi ... ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Device Connected.");
  digitalWrite(LED, LOW);

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("ERROR - SD Card initialization failed!");
    return;
  }

  // initialize KALO I2S Recording Services
  I2S_Record_Init();

  // INIT done, starting user interaction
  Serial.println("> HOLD button for recording AUDIO .. RELEASE button for REPLAY & Deepgram transcription");
}

void loop() {
  if (digitalRead(pin_RECORD_BTN) == LOW) {
    digitalWrite(LED, HIGH);
    delay(30);

    // Start Recording
    Record_Start(AUDIO_FILE);
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH) {
    digitalWrite(LED, LOW);

    float recorded_seconds;
    if (Record_Available(AUDIO_FILE, &recorded_seconds)) {
      if (recorded_seconds > 0.4) {  // ignore short btn TOUCH
        digitalWrite(LED, HIGH);

        String transcription = SpeechToText_Deepgram(AUDIO_FILE);

        digitalWrite(LED, LOW);
        Serial.println(transcription);

        transcription.toLowerCase();

        // Send the transcription to the responder board
        sendCommandToResponder(transcription);

        // Optionally, log the transcription
        Serial.println("Command sent to responder: " + transcription);
      }
    }
  }

  // Keep the Deepgram API connection alive during idle periods
  if (digitalRead(pin_RECORD_BTN) == HIGH) {
    static uint32_t millis_ping_before;
    if (millis() > (millis_ping_before + 5000)) {
      millis_ping_before = millis();
      digitalWrite(LED, HIGH);
      Deepgram_KeepAlive();
    }
  }
}

// Function to send transcription to the responder board via TCP
void sendCommandToResponder(String command) {
  WiFiClient client;

  if (client.connect(responder_ip, responder_port)) {
    Serial.println("Connected to responder");
    client.println(command);
    delay(500);
    client.stop();
    Serial.println("Disconnected from responder");
  } else {
    Serial.println("Failed to connect to responder");
  }
}
