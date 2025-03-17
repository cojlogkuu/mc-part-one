#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

const char *ssid = "dodkolox";
const char *password = "qwerty1234";

const uint8_t redLED = 2; // GPIO2 (D4)
const uint8_t yellowLED = 14; // GPIO14 (D5)
const uint8_t greenLED = 13; // GPIO13 (D7)
const uint8_t button = 12; // GPIO12 (D6)

typedef enum {
  RED,
  YELLOW,
  GREEN
} LEDColor;

bool reversed = false;
LEDColor currentLED = RED;
uint32_t currentTime;
uint16_t blinkInterval = 1000;
uint32_t previousBlinkTime = 0;

bool buttonState;
bool buttonWasPressed = false;
uint32_t pressStartTime = 0;
uint16_t holdInterval = 1000;

AsyncWebServer server(80);

void handleHold(AsyncWebServerRequest *request) {
  Serial.println("WEB button was held");
  reversed = true;
  request->send_P(200, "text/html", "ok");
}

void handleReleased(AsyncWebServerRequest *request) {
  Serial.println("WEB button was released");
  reversed = false;
  request->send_P(200, "text/html", "ok");
}

void pinSetup() {
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(button, INPUT_PULLUP);
}

void serverSetup() {
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to wife");
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(".");
  }

  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/hold", HTTP_GET, handleHold);
  server.on("/release", HTTP_GET, handleReleased);
  server.begin();
  Serial.print("\nConnected to WiFi: ");
  Serial.println(WiFi.localIP());
}

void turnOffLEDs() {
  digitalWrite(redLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, LOW);
}

void blinkLEDs() {
  currentTime = millis();

  if (currentTime - previousBlinkTime >= blinkInterval) {
    previousBlinkTime = currentTime;
    turnOffLEDs();

    switch (currentLED) {
      case 0: digitalWrite(redLED, HIGH);
        break;
      case 1: digitalWrite(yellowLED, HIGH);
        break;
      case 2: digitalWrite(greenLED, HIGH);
        break;
      default: ;
    }

    if (reversed) {
      currentLED = (LEDColor)((currentLED - 1 + 3) % 3);
    } else {
      currentLED = (LEDColor)((currentLED + 1) % 3);
    }
  }
}

void handleButtonHold() {
  buttonState = digitalRead(button);

  if (buttonState) {
    if (!buttonWasPressed) {
      buttonWasPressed = true;
      pressStartTime = millis();
    } else if (millis() - pressStartTime >= holdInterval) {
      reversed = true;
    }
  } else {
    if (buttonWasPressed) {
      reversed = false;
    }
    buttonWasPressed = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinSetup();
  turnOffLEDs();
  serverSetup();
}

void loop() {
  handleButtonHold();
  blinkLEDs();
}
