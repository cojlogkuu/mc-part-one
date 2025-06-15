#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SSID "dodkolox"
#define PASSWORD "qwerty1234"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClientSecure client;

const char *baseUrl = "https://api.coinlore.net/api/ticker/?id=";
char url[64];

uint8_t currenciesId[5] = {90, 80, 2, 1, 10};
uint8_t currentPage = 0;

typedef struct {
  char percent_change_24h[32];
  char name[32];
  char price_usd[32];
} CurrencyData;

CurrencyData currency;

const uint16_t interval = 1000;
bool wifiWasConnected = false;

void showWifiConnection() {
  static uint8_t dotCount = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    dotCount = (dotCount + 1) % 4;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 28);
    display.print("Connecting to WiFi");
    for (uint8_t i = 0; i < dotCount; i++) {
      display.print(".");
    }
    display.display();
  }
}

bool fetchData() {
  HTTPClient https;
  client.setInsecure();

  snprintf(url, sizeof(url), "%s%d", baseUrl, currenciesId[currentPage]);

  if (https.begin(client, url)) {
    int httpsCode = https.GET();

    if (httpsCode > 0) {
      String payload = https.getString();

      StaticJsonDocument<1024> doc;
      DeserializationError err = deserializeJson(doc, payload);

      if (!err) {
        JsonArray arr = doc.as<JsonArray>();
        JsonObject obj = arr[0];

        strlcpy(currency.percent_change_24h, obj["percent_change_24h"] | "", sizeof(currency.percent_change_24h));
        strlcpy(currency.name, obj["name"] | "", sizeof(currency.name));
        strlcpy(currency.price_usd, obj["price_usd"] | "", sizeof(currency.price_usd));

        https.end();
        return true;
      }
    }
    https.end();
  }

  return false;
}

void showNextPage() {
  if (!fetchData()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Failed to load data");
    display.display();
    return;
  }

  char buffer[64];

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  snprintf(buffer, sizeof(buffer), "%s%% in 24h", currency.percent_change_24h);
  display.setCursor(10, 0);
  display.print(buffer);

  display.setTextSize(2);

  display.setCursor(0, 16);
  display.print(currency.name);

  snprintf(buffer, sizeof(buffer), "%s$", currency.price_usd);
  display.setCursor(0, 40);
  display.print(buffer);

  display.display();

  currentPage = (currentPage + 1) % 5;
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiWasConnected = false;
    showWifiConnection();
    return;
  }

  if (!wifiWasConnected) {
    showNextPage();
    wifiWasConnected = true;
  }
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
    while (true);
  }

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    showWifiConnection();
  }
}

void loop() {
  checkWifi();
}
