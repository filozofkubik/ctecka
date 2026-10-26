// Zdrojovy kod pro desku LaskaKit ESPink-Frame-7.x
// Popis desky: https://www.laskakit.cz/laskakit-espink-frame-7-x-e-paper-board/
// Projekt desky: https://github.com/LaskaKit/ESPink-Frame-7x
// Pouzity displej: GDEM075F52
/*
# Restartuje hardware starající se o COM3 (odpojí a znovu připojí zařízení v systému)
Get-CimInstance Win32_PnPEntity | Where-Object { $_.Caption -match 'COM3' } | Disable-PnpDevice -Confirm:$false; Get-CimInstance Win32_PnPEntity | Where-Object { $_.Caption -match 'COM3' } | Enable-PnpDevice -Confirm:$false


*/
#include <Arduino.h>
#include <GxEPD2_4C.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>

namespace {

constexpr char wifiSsid[] = "k";
constexpr char wifiPassword[] = "hhh123456";
constexpr char serverUrl[] = "http://192.168.1.100:8080/api/data";

constexpr uint8_t epdMosi = 11;
constexpr uint8_t epdSck = 12;
constexpr uint8_t epdCs = 10;
constexpr uint8_t epdDc = 48;
constexpr uint8_t epdRst = 45;
constexpr uint8_t epdBusy = 38;
constexpr uint8_t peripheralPower = 47;

constexpr unsigned long refreshIntervalMs = 60UL * 60UL * 1000UL;

GxEPD2_4C<GxEPD2_750c_GDEM075F52,
          GxEPD2_750c_GDEM075F52::HEIGHT / 2>
    display(GxEPD2_750c_GDEM075F52(epdCs, epdDc, epdRst, epdBusy));

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

void fetchServerData() {
  if (!connectToWifi()) {
    return;
  }

  HTTPClient http;
  if (!http.begin(serverUrl)) {
    Serial.println("Nepodarilo se pripravit HTTP pozadavek.");
    return;
  }

  const int statusCode = http.GET();
  if (statusCode < 0) {
    Serial.print("HTTP pozadavek selhal: ");
    Serial.println(http.errorToString(statusCode));
  } else if (statusCode >= 200 && statusCode < 300) {
    Serial.println("Odpoved serveru:");
    Serial.println(http.getString());
  } else {
    Serial.print("Server vratil HTTP status: ");
    Serial.println(statusCode);
  }

  http.end();
}

void clearDisplay() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

void drawGreeting() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(4);
    display.setCursor(20, 20);
    display.print("Ahoj!");
    display.setCursor(40, 40);
    display.print("Ahoj!");
  } while (display.nextPage());
}

void refreshDisplay() {
  clearDisplay();
  drawGreeting();
  Serial.println("Displej byl vycisten a znovu vykreslen.");
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
  display.setFullWindow();
  refreshDisplay();
  fetchServerData();
}

void loop() {
  static unsigned long lastRefreshMs = millis();
  const unsigned long now = millis();

  if (now - lastRefreshMs >= refreshIntervalMs) {
    fetchServerData();
    refreshDisplay();
    lastRefreshMs = now;
  }
}
