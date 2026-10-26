// LaskaKit ESPink-Frame-7.x, displej GDEM075F52.

#include <Arduino.h>
#include <GxEPD2_4C.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>

namespace {

constexpr char wifiSsid[] = "k";
constexpr char wifiPassword[] = "hhh123456";
constexpr uint16_t logServerPort = 5500;
constexpr char logFilePath[] = "/logs.txt";

constexpr uint8_t epdMosi = 11;
constexpr uint8_t epdSck = 12;
constexpr uint8_t epdCs = 10;
constexpr uint8_t epdDc = 48;
constexpr uint8_t epdRst = 45;
constexpr uint8_t epdBusy = 38;
constexpr uint8_t peripheralPower = 47;

constexpr unsigned long wifiRetryIntervalMs = 10000UL;
constexpr unsigned long displayRefreshIntervalMs = 60000UL;
constexpr size_t maxLogFileSize = 64 * 1024;

GxEPD2_4C<GxEPD2_750c_GDEM075F52,
          GxEPD2_750c_GDEM075F52::HEIGHT / 2>
    display(GxEPD2_750c_GDEM075F52(epdCs, epdDc, epdRst, epdBusy));
WiFiServer logServer(logServerPort);

bool logStorageReady = false;
bool serverStarted = false;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastDisplayRefreshMs = 0;

void logMessage(const String& message) {
  String line = String(millis()) + " ms: " + message + "\n";
  Serial.print(line);

  if (!logStorageReady) {
    return;
  }

  File file = SPIFFS.open(logFilePath, FILE_APPEND);
  if (!file) {
    Serial.println("Chyba: nelze otevrit soubor s logem.");
    return;
  }

  if (file.size() + line.length() > maxLogFileSize) {
    file.close();
    if (!SPIFFS.remove(logFilePath)) {
      Serial.println("Chyba: nelze smazat stary soubor s logem.");
      return;
    }
    file = SPIFFS.open(logFilePath, FILE_WRITE);
    if (!file) {
      Serial.println("Chyba: nelze vytvorit novy soubor s logem.");
      return;
    }
    line = String(millis()) + " ms: log zacal znovu po dosazeni limitu\n" +
           line;
  }

  if (file.print(line) != line.length()) {
    Serial.println("Chyba: log se nepodarilo cely ulozit.");
  }
  file.close();
}

void drawStatusDisplay() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const String ipAddress =
      connected ? WiFi.localIP().toString() : String("nedostupna");

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(3);
    display.setCursor(20, 50);
    display.print(connected ? "Wi-Fi: PRIPOJENO" : "Wi-Fi: NEPRIPOJENO");
    display.setCursor(20, 115);
    display.print("IP: ");
    display.print(ipAddress);
    display.setCursor(20, 180);
    display.print("TCP port: ");
    display.print(logServerPort);
    display.setCursor(20, 245);
    display.print(serverStarted ? "Server: BEZI" : "Server: CEKA NA WI-FI");
  } while (display.nextPage());
}

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifiSsid, wifiPassword);
  lastWifiAttemptMs = millis();
  logMessage("Pripojuji se k Wi-Fi.");
}

void maintainWifiAndServer() {
  const unsigned long now = millis();
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected && !serverStarted) {
    logServer.begin();
    serverStarted = true;
    logMessage("Wi-Fi pripojena. IP: " + WiFi.localIP().toString() +
               ", log server bezi na portu " + String(logServerPort) + ".");
  } else if (!connected && serverStarted) {
    logServer.end();
    serverStarted = false;
    logMessage("Wi-Fi spojeni bylo ztraceno.");
  }

  if (!connected && now - lastWifiAttemptMs >= wifiRetryIntervalMs) {
    WiFi.disconnect();
    WiFi.begin(wifiSsid, wifiPassword);
    lastWifiAttemptMs = now;
    logMessage("Opakuji pripojeni k Wi-Fi.");
  }
}

void sendLogFile(WiFiClient& client) {
  if (!logStorageReady) {
    return;
  }

  File file = SPIFFS.open(logFilePath, FILE_READ);
  if (!file) {
    return;
  }

  uint8_t buffer[512];
  while (file.available() && client.connected()) {
    const size_t bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead == 0 || client.write(buffer, bytesRead) != bytesRead) {
      break;
    }
  }
  file.close();
}

void handleTcpClient() {
  WiFiClient client = logServer.accept();
  if (!client) {
    return;
  }

  client.setTimeout(2000);
  String command = client.readStringUntil('\n');
  command.trim();

  if (command == "log") {
    sendLogFile(client);
  }
  client.stop();
}

}  // namespace

void setup() {
  Serial.begin(115200);

  if (SPIFFS.begin(true)) {
    logStorageReady = true;
  } else {
    Serial.println("Chyba: SPIFFS se nepodarilo pripojit.");
  }
  logMessage("Zarizeni se spousti.");

  pinMode(peripheralPower, OUTPUT);
  digitalWrite(peripheralPower, HIGH);
  delay(100);

  SPI.begin(epdSck, 13, epdMosi, epdCs);
  display.init();
  display.setRotation(1);
  display.setFullWindow();

  startWifi();
  drawStatusDisplay();
  lastDisplayRefreshMs = millis();
}

void loop() {
  maintainWifiAndServer();
  handleTcpClient();

  const unsigned long now = millis();
  if (now - lastDisplayRefreshMs >= displayRefreshIntervalMs) {
    drawStatusDisplay();
    lastDisplayRefreshMs = now;

    const bool connected = WiFi.status() == WL_CONNECTED;
    logMessage(connected
                   ? "Stav: Wi-Fi pripojena, IP " +
                         WiFi.localIP().toString() + ", TCP port " +
                         String(logServerPort) + "."
                   : "Stav: Wi-Fi nepripojena.");
  }

  delay(1);
}
