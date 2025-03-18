#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define SSID "dodkolox"
#define PASSWORD "qwerty1234"

typedef struct led_s {
  uint8_t pin;
  bool state;
  led_s *next;
  led_s *prev;
} led_t;

typedef struct button_s {
  uint8_t pin;
  bool state;
  bool wasPressed;
} button_t;

led_t redLED = {D4, LOW, nullptr, nullptr}; // GPIO2
led_t yellowLED = {D5, LOW, nullptr, nullptr}; // GPIO14
led_t greenLED = {D7, LOW, nullptr, &yellowLED}; // GPIO13

button_t button = {D6, LOW, false}; //GPIO12

led_t *currentLED = &redLED;
bool reversedOrder = false;
uint32_t currentTime;
uint16_t blinkInterval = 1000;
uint32_t previousBlinkTime = 0;

uint32_t pressStartTime = 0;
uint16_t holdInterval = 500;

AsyncWebServer server(80);

void handleHold(AsyncWebServerRequest *request) {
  Serial.println("WEB button was held");
  reversedOrder = true;
  request->send_P(200, "text/html", "ok");
}

void handleReleased(AsyncWebServerRequest *request) {
  Serial.println("WEB button was released");
  reversedOrder = false;
  request->send_P(200, "text/html", "ok");
}

void setupLEDOrder() {
  redLED.next = &yellowLED;
  redLED.prev = &greenLED;

  yellowLED.next = &greenLED;
  yellowLED.prev = &redLED;

  greenLED.next = &redLED;
  greenLED.prev = &yellowLED;
}

void pinSetup() {
  pinMode(redLED.pin, OUTPUT);
  pinMode(yellowLED.pin, OUTPUT);
  pinMode(greenLED.pin, OUTPUT);
  pinMode(button.pin, INPUT_PULLUP);
}

void serverSetup() {
  WiFi.begin(SSID, PASSWORD);
  Serial.print("\nConnecting to wife");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/hold", HTTP_GET, handleHold);
  server.on("/release", HTTP_GET, handleReleased);
  server.begin();
  Serial.println("Connected to WiFi: ");
  Serial.println(WiFi.localIP());
}

void lightLEDs() {
  digitalWrite(redLED.pin, redLED.state);
  digitalWrite(yellowLED.pin, yellowLED.state);
  digitalWrite(greenLED.pin, greenLED.state);
}

void lightNextLED() {
  currentTime = millis();

  if (currentTime - previousBlinkTime >= blinkInterval) {
    previousBlinkTime = currentTime;

    currentLED->state = LOW;
    currentLED = reversedOrder ? currentLED->prev : currentLED->next;
    currentLED->state = HIGH;

    lightLEDs();
  }
}

void handleButtonHold() {
  button.state = digitalRead(button.pin);

  if (button.state) {
    if (!button.wasPressed) {
      button.wasPressed = true;
      pressStartTime = millis();
    } else if (millis() - pressStartTime >= holdInterval) {
      reversedOrder = true;
    }
  } else {
    if (button.wasPressed) {
      reversedOrder = false;
    }
    button.wasPressed = false;
  }
}

void setup() {
  Serial.begin(115200);
  setupLEDOrder();
  pinSetup();
  serverSetup();
}

void loop() {
  handleButtonHold();
  lightNextLED();
}
