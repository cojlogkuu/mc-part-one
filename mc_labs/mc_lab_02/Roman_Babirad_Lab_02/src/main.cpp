#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define SSID "iPhone Bohdan"
#define PASSWORD "135792468"
#define BLINK_INTERVAL 1000
#define HOLD_INTERVAL 500

enum class Color {
  RED,
  YELLOW,
  GREEN
};

typedef struct led_s {
  const uint8_t pin;
  bool state;
  led_s *next;
  led_s *prev;
  Color color;
} led_t;

typedef struct button_s {
  uint8_t pin;
  bool state;
  bool wasPressed;
  uint32_t pressStartTime;
  bool hardIsHeld;
  bool webIsHeld;
  bool serialIsHeld;
} button_t;

led_t redLED = {D4, LOW, nullptr, nullptr, Color::RED}; // GPIO2
led_t yellowLED = {D5, LOW, nullptr, nullptr, Color::YELLOW}; // GPIO14
led_t greenLED = {D7, LOW, nullptr, nullptr, Color::GREEN}; // GPIO13

button_t button = {D6, LOW, false, 0, false, false, false}; //GPIO12

led_t *currentLED = &redLED;
uint32_t currentTime;
uint32_t previousBlinkTime = 0;
uint8_t serialData;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleHold(AsyncWebServerRequest *request) {
  button.webIsHeld = true;
  request->send_P(200, "text/html", "ok");
}

void handleReleased(AsyncWebServerRequest *request) {
  button.webIsHeld = false;
  request->send_P(200, "text/html", "ok");
}

void handleStartAlgo2(AsyncWebServerRequest *request) {
  Serial.print("n");
  request->send_P(200, "text/html", "ok");
}

void handleStopAlgo2(AsyncWebServerRequest *request) {
  Serial.print("f");
  request->send_P(200, "text/html", "ok");
}

void sendCurrentLEDtoWEB() {
  switch (currentLED->color) {
    case Color::RED:
      ws.textAll("red");
      break;
    case Color::YELLOW:
      ws.textAll("yellow");
      break;
    case Color::GREEN:
      ws.textAll("green");
      break;
  }
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

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.print(WiFi.localIP());
  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/hold", HTTP_GET, handleHold);
  server.on("/release", HTTP_GET, handleReleased);
  server.on("/startAlgo2", HTTP_GET, handleStartAlgo2);
  server.on("/stopAlgo2", HTTP_GET, handleStopAlgo2);
  server.addHandler(&ws);
  server.begin();
}

void lightLEDs() {
  digitalWrite(redLED.pin, redLED.state);
  digitalWrite(yellowLED.pin, yellowLED.state);
  digitalWrite(greenLED.pin, greenLED.state);
}

void lightNextLED() {
  currentTime = millis();

  if (currentTime - previousBlinkTime >= BLINK_INTERVAL) {
    previousBlinkTime = currentTime;

    currentLED->state = LOW;
    currentLED = button.hardIsHeld || button.webIsHeld || button.serialIsHeld ? currentLED->prev : currentLED->next;
    currentLED->state = HIGH;

    sendCurrentLEDtoWEB();
    lightLEDs();
  }
}

void handleButtonHold() {
  button.state = digitalRead(button.pin) == LOW;

  if (button.state) {
    if (!button.wasPressed) {
      button.wasPressed = true;
      button.pressStartTime = millis();
    } else if (millis() - button.pressStartTime >= HOLD_INTERVAL) {
      button.hardIsHeld = true;
    }
  } else {
    if (button.wasPressed) {
      button.hardIsHeld = false;
    }
    button.wasPressed = false;
  }
}

void checkSerial() {
  if (Serial.available() > 0) {
    serialData = Serial.read();
    switch (serialData) {
      case 'n':
        button.serialIsHeld = true;
        break;
      case 'f':
        button.serialIsHeld = false;
        break;
    }
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  setupLEDOrder();
  pinSetup();
  serverSetup();
}

void loop() {
  handleButtonHold();
  checkSerial();
  lightNextLED();
  ws.cleanupClients();
}
