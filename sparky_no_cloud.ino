#include <Arduino.h>
#include <WiFi.h>
#include "esp_bt.h"

static const char* WIFI_SSID = "";
static const char* WIFI_PASS = "";

static const uint16_t TCP_PORT = 3602;
static const uint32_t DSMR_BAUD = 115200;
static const uint32_t DSMR_CFG  = SERIAL_8N1;

static const int P1_RX_PIN = 10; 
static const int P1_TX_PIN = -1;

static const int PIN_B = 5;
static const int PIN_G = 6;
static const int PIN_R = 7;

static const uint32_t NO_TG_TIMEOUT_MS = 8000;
static const uint32_t BLINK_MS = 300;

static inline void ledOff() {
  ledSet(false, false, false);
}

static inline void ledSet(bool r, bool g, bool b) {
  if (r) { analogWrite(PIN_R, 245); } else { analogWrite(PIN_R, 255); }
  if (g) { analogWrite(PIN_G, 245); } else { analogWrite(PIN_G, 255); }
  if (b) { analogWrite(PIN_B, 245); } else { analogWrite(PIN_B, 255); }
}

WiFiServer server(TCP_PORT);
WiFiClient client;

static String curTg;
static String lastGoodTg;
static bool inTelegram = false;
static bool sawBang = false;
static uint32_t lastTelegramMs = 0;
static uint32_t lastBlinkMs = 0;
static bool blinkOn = false;

static void wifiEnsureConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_2dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  bool blink = false;

  while (!WiFi.isConnected()) {
    blink = !blink;
    ledSet(blink, false, false);
    delay(BLINK_MS);
  }
}

static void updateLed() {
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);
  if (!wifiOk) {
    ledSet(true, false, false);
    return;
  }

  const bool telegramOk = (millis() - lastTelegramMs) <= NO_TG_TIMEOUT_MS;
  if (!telegramOk) {
    ledSet(false, false, true);
    return;
  }

  const bool clientOk = client && client.connected();
  if (clientOk) {
    if (millis() - lastBlinkMs >= BLINK_MS) {
      lastBlinkMs = millis();
      blinkOn = !blinkOn;
    }
    ledSet(false, blinkOn, false);
  } else {
    ledSet(false, true, false);
  }
}

static void acceptClientIfNeeded() {
  if (client && client.connected()) return;

  WiFiClient newClient = server.available();
  if (!newClient) return;

  if (client) client.stop();
  client = newClient;
  client.setNoDelay(true);

  if (lastGoodTg.length() > 0) {
    client.write((const uint8_t*)lastGoodTg.c_str(), lastGoodTg.length());
  }

  inTelegram = false;
  sawBang = false;
  curTg = "";
}

static inline void publishTelegramIfClient(const String& tg) {
  if (client && client.connected()) {
    client.write((const uint8_t*)tg.c_str(), tg.length());
  }
}

static void pumpDsmr() {
  while (Serial1.available() > 0) {
    char c = (char)Serial1.read();

    if (!inTelegram) {
      if (c == '/') {
        inTelegram = true;
        sawBang = false;
        curTg = "";
        curTg += c;
      }
      continue;
    }

    curTg += c;

    if (curTg.length() > 2500) {
      inTelegram = false;
      sawBang = false;
      curTg = "";
      continue;
    }

    if (c == '!') {
      sawBang = true;
      continue;
    }

    if (sawBang && (c == '\n')) {
      lastGoodTg = curTg;
      lastTelegramMs = millis();
      publishTelegramIfClient(lastGoodTg);

      inTelegram = false;
      sawBang = false;
      curTg = "";
      continue;
    }

    if (c == '/') {
      inTelegram = true;
      sawBang = false;
      curTg = "";
      curTg += c;
    }
  }
}

void setup() {
  setCpuFrequencyMhz(80);
  esp_bt_controller_disable();
  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  ledOff();

  wifiEnsureConnected();

  server.begin();
  server.setNoDelay(true);

  // DSMR UART
  Serial1.begin(DSMR_BAUD, DSMR_CFG, P1_RX_PIN, P1_TX_PIN);

  lastTelegramMs = millis();
}

void loop() {
  wifiEnsureConnected();

  acceptClientIfNeeded();
  pumpDsmr();
  updateLed();

  if (client && !client.connected()) {
    client.stop();
  }

  delay(2);
}
