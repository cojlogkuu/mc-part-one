#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SSID "TP-LINK_tehkomp"
#define PASSWORD "sambir1241"
#define MQTT_SEVER "test.mosquitto.org"
#define TOPIC_LED "Kp6393TL6j59989XEdlh/led/"
#define TOPIC_MODE "Kp6393TL6j59989XEdlh/mode"

typedef struct led_s {
  uint8_t pin;
  bool state;
  led_s *next;
  led_s *prev;
} led_t;

led_t led1 = {D1, LOW, nullptr, nullptr};
led_t led2 = {D2, LOW, nullptr, nullptr};
led_t led3 = {D5, LOW, nullptr, nullptr};
led_t led4 = {D6, LOW, nullptr, nullptr};
led_t led5 = {D7, LOW, nullptr, nullptr};

led_t *leds[] = {&led1, &led2, &led3, &led4, &led5};

led_t *currentLED = nullptr;
uint16_t blinkInterval = 1000;
uint32_t previousBlinkTime = 0;
bool allLedState = LOW;
uint8_t ledsNumber = 5;

enum LedMode {
  MODE_NONE = 0,
  MODE_ALL,
  MODE_NORMAL,
  MODE_REVERSE
};

LedMode currentMode = MODE_NONE;

WiFiClient espClient;
PubSubClient client(espClient);

void setupLedsOrder() {
  led1.next = &led2;
  led1.prev = &led5;

  led2.next = &led3;
  led2.prev = &led1;

  led3.next = &led4;
  led3.prev = &led2;

  led4.next = &led5;
  led4.prev = &led3;

  led5.next = &led1;
  led5.prev = &led4;
}

void pinSetup() {
  pinMode(led1.pin, OUTPUT);
  pinMode(led2.pin, OUTPUT);
  pinMode(led3.pin, OUTPUT);
  pinMode(led4.pin, OUTPUT);
  pinMode(led5.pin, OUTPUT);
}

void turnOffLeds() {
  digitalWrite(led1.pin, LOW);
  digitalWrite(led2.pin, LOW);
  digitalWrite(led3.pin, LOW);
  digitalWrite(led4.pin, LOW);
  digitalWrite(led5.pin, LOW);
}

void lightLeds() {
  digitalWrite(led1.pin, led1.state);
  digitalWrite(led2.pin, led2.state);
  digitalWrite(led3.pin, led3.state);
  digitalWrite(led4.pin, led4.state);
  digitalWrite(led5.pin, led5.state);
}

void callback(char *topic, byte *payload, unsigned int length) {
  char message[10] = {0};
  strncpy(message, (char *) payload, min(length, sizeof(message) - 1));

  if (strcmp(topic, TOPIC_MODE) == 0) {
    turnOffLeds();
    currentLED = nullptr;

    if (strcmp(message, "all") == 0) {
      currentMode = MODE_ALL;
    } else if (strcmp(message, "normal") == 0) {
      currentMode = MODE_NORMAL;
    } else if (strcmp(message, "reverse") == 0) {
      currentMode = MODE_REVERSE;
    } else {
      currentMode = MODE_NONE;
    }
  }

  for (int i = 0; i < ledsNumber; i++) {
    char topicBuf[64];
    snprintf(topicBuf, sizeof(topicBuf), "%s%d", TOPIC_LED, i + 1);

    if (strcmp(topic, topicBuf) == 0) {
      bool state = strcmp(message, "on") == 0;
      leds[i]->state = state;
      break;
    }
  }
}

void connectionSetup() {
  WiFi.begin(SSID, PASSWORD);
  Serial.print("\nConnecting to wife");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.print("\nConnected");
  client.setServer(MQTT_SEVER, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("esp8266_Kp6393TL6j59989XEdlh")) {
      client.subscribe(TOPIC_MODE);
      for (int i = 1; i <= ledsNumber; i++) {
        char topicBuf[64];
        snprintf(topicBuf, sizeof(topicBuf), "%s%d", TOPIC_LED, i);
        client.subscribe(topicBuf);
      }
    } else {
      Serial.print(client.state());
    }
  }
}

void allLedsBlinking() {
  if (millis() - previousBlinkTime > blinkInterval) {
    previousBlinkTime = millis();
    allLedState = !allLedState;
    for (int i = 0; i < ledsNumber; i++) {
      digitalWrite(leds[i]->pin, allLedState);
    }
  }
}

void normalOrderBlinkig() {
  if (millis() - previousBlinkTime > blinkInterval) {
    previousBlinkTime = millis();
    turnOffLeds();
    if (!currentLED) currentLED = &led1;
    digitalWrite(currentLED->pin, HIGH);
    currentLED = currentLED->next;
  }
}

void reverseOrderBlinking() {
  if (millis() - previousBlinkTime > blinkInterval) {
    previousBlinkTime = millis();
    turnOffLeds();
    if (!currentLED) currentLED = &led5;
    digitalWrite(currentLED->pin, HIGH);
    currentLED = currentLED->prev;
  }
}

void setup() {
  Serial.begin(115200);
  setupLedsOrder();
  pinSetup();
  connectionSetup();
  turnOffLeds();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  switch (currentMode) {
    case MODE_ALL:
      allLedsBlinking();
      break;
    case MODE_NORMAL:
      normalOrderBlinkig();
      break;
    case MODE_REVERSE:
      reverseOrderBlinking();
      break;
    case MODE_NONE:
      lightLeds();
    default:
      break;
  }
}
