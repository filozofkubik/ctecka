#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_4C.h>
#include <WiFi.h>
#include <WiFiClient.h>

namespace {

constexpr char wifiSsid[] = "k";
constexpr char wifiPassword[] = "hhh123456";
const IPAddress serverIp(192, 168, 88, 252);
constexpr uint16_t serverPort = 5000;
constexpr char echoRequest[] = "echo test z ESP32";

constexpr uint8_t epdMosi = 11;
constexpr uint8_t epdSck = 12;
constexpr uint8_t epdCs = 10;
constexpr uint8_t epdDc = 48;
constexpr uint8_t epdRst = 45;
constexpr uint8_t epdBusy = 38;
constexpr uint8_t peripheralPower = 47;
constexpr unsigned long serverCycleIntervalMs = 10UL * 1000UL;
constexpr unsigned long renderingIntervalMs = 10UL * 60UL * 1000UL;
constexpr unsigned long screenUpdateIntervalMs = 10UL * 60UL * 1000UL;
constexpr unsigned long fullRefreshIntervalMs = 60UL * 60UL * 1000UL;
constexpr int16_t counterTextX = 212;
constexpr int16_t counterTextY = 50;
constexpr uint16_t counterWindowX = 208;
constexpr uint16_t counterWindowY = 48;
constexpr uint16_t counterWindowWidth = 128;
constexpr uint16_t counterWindowHeight = 24;
constexpr uint16_t partialUpdatesPerFullRefresh = 120;

GxEPD2_4C<GxEPD2_750c_GDEM075F52,
          GxEPD2_750c_GDEM075F52::HEIGHT>
    display(GxEPD2_750c_GDEM075F52(epdCs, epdDc, epdRst, epdBusy));

uint32_t counter = 0;
uint8_t counterColorIndex = 0;
unsigned long lastCounterUpdateMs = 0;
unsigned long lastFullRefreshMs = 0;
unsigned long lastServerCycleMs = 0;
unsigned long lastRenderingMs = 0;
uint16_t partialUpdateCount = 0;
constexpr uint16_t counterColors[] = {
    GxEPD_BLACK, GxEPD_RED, GxEPD_YELLOW};

bool connectToWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("Pripojuji se k Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  const unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startMs < 20000UL) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nPripojeni k Wi-Fi se nepodarilo.");
    return false;
  }

  Serial.print("\nWi-Fi pripojena, IP adresa: ");
  Serial.println(WiFi.localIP());
  return true;
}

void testServerEcho() {
  if (!connectToWifi()) {
    return;
  }

  WiFiClient client;
  Serial.print("Pripojuji se k TCP serveru ");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);
  if (!client.connect(serverIp, serverPort)) {
    Serial.println("Pripojeni k TCP serveru se nepodarilo.");
    return;
  }

  Serial.print("Odesilam pozadavek: ");
  Serial.println(echoRequest);
  client.print(echoRequest);

  size_t responseBytes = 0;
  const unsigned long responseStartMs = millis();
  Serial.print("Odpoved serveru: ");
  while ((client.connected() || client.available()) &&
         millis() - responseStartMs < 10000UL) {
    while (client.available()) {
      Serial.write(client.read());
      responseBytes++;
    }
    delay(1);
  }

  if (responseBytes == 0) {
    Serial.println("<bez odpovedi>");
  } else {
    Serial.println();
    Serial.print("Prijato bajtu: ");
    Serial.println(responseBytes);
  }
  client.stop();
}

void renderContent() {
  // Pripraveno pro budoucí vykreslování.
}

void drawCounter(bool fullRefresh) {
  if (fullRefresh) {
    display.setFullWindow();
  } else {
    display.setPartialWindow(
        counterWindowX, counterWindowY,
        counterWindowWidth, counterWindowHeight);
  }

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    if (fullRefresh) {
      display.setTextSize(2);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(20, counterTextY);
      display.print("Aktualni citac: ");
    }
    display.setTextSize(2);
    display.setTextColor(counterColors[counterColorIndex]);
    display.setCursor(counterTextX, counterTextY);
    display.print(counter);
  } while (display.nextPage());

  Serial.print("Aktualni citac: ");
  Serial.println(counter);
}

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(peripheralPower, OUTPUT);
  digitalWrite(peripheralPower, HIGH);
  delay(100);

  SPI.begin(epdSck, 13, epdMosi, epdCs);
  display.init();
  display.setRotation(1);
  drawCounter(true);
  lastCounterUpdateMs = millis();
  lastFullRefreshMs = lastCounterUpdateMs;
  lastRenderingMs = lastCounterUpdateMs;
  testServerEcho();
  lastServerCycleMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastCounterUpdateMs >= screenUpdateIntervalMs) {
    const unsigned long elapsedIntervals =
        (now - lastCounterUpdateMs) / screenUpdateIntervalMs;
    counter += elapsedIntervals;
    lastCounterUpdateMs += elapsedIntervals * screenUpdateIntervalMs;
    counterColorIndex =
        (counterColorIndex + elapsedIntervals) %
        (sizeof(counterColors) / sizeof(counterColors[0]));

    const bool fullRefreshDue =
        now - lastFullRefreshMs >= fullRefreshIntervalMs ||
        partialUpdateCount >= partialUpdatesPerFullRefresh;
    drawCounter(fullRefreshDue);

    if (fullRefreshDue) {
      lastFullRefreshMs = millis();
      partialUpdateCount = 0;
    } else {
      partialUpdateCount++;
    }
  }

  if (now - lastServerCycleMs >= serverCycleIntervalMs) {
    testServerEcho();
    lastServerCycleMs = millis();
  }

  if (now - lastRenderingMs >= renderingIntervalMs) {
    renderContent();
    lastRenderingMs = now;
  }
}
