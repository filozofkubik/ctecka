// LaskaKit ESPink-Frame-7.x, displej GDEM075F52.

#include <Arduino.h>
#include <GxEPD2_4C.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "ping/ping_sock.h"
#include <utility>
#include <vector>

namespace {

constexpr char wifiSsid[] = "k";
constexpr char wifiPassword[] = "hhh123456";
constexpr uint16_t logServerPort = 5500;
constexpr char logFilePath[] = "/logs.txt";
const IPAddress fileServerAddress(192, 168, 78, 253);
constexpr uint16_t fileServerPort = 5000;

constexpr uint8_t epdMosi = 11;
constexpr uint8_t epdSck = 12;
constexpr uint8_t epdCs = 10;
constexpr uint8_t epdDc = 48;
constexpr uint8_t epdRst = 45;
constexpr uint8_t epdBusy = 38;
constexpr uint8_t peripheralPower = 47;
constexpr uint8_t upButtonPin = 39;
constexpr uint8_t setButtonPin = 40;

constexpr unsigned long wifiRetryIntervalMs = 10000UL;
constexpr unsigned long fileRefreshIntervalMs = 10000UL;
constexpr unsigned long buttonDebounceMs = 40UL;
constexpr unsigned long tcpResponseTimeoutMs = 10000UL;
constexpr size_t maxLogFileSize = 64 * 1024;

GxEPD2_4C<GxEPD2_750c_GDEM075F52,
          GxEPD2_750c_GDEM075F52::HEIGHT / 2>
    display(GxEPD2_750c_GDEM075F52(epdCs, epdDc, epdRst, epdBusy));
WiFiServer logServer(logServerPort);

struct CachedFile {
  String name;
  std::vector<uint8_t> contents;
};

enum class DisplayMode {
  FileName,
  Diagnostics,
};

std::vector<CachedFile> cachedFiles;
size_t currentFileIndex = 0;
DisplayMode displayMode = DisplayMode::FileName;
String noFilesReason = "Cekam na pripojeni k Wi-Fi";
String lastFileRequest = "zadny";
String lastFileServerResult = "Zatim bez pokusu";
size_t lastFileResponseBytes = 0;
unsigned long lastFileRequestDurationMs = 0;
String lastPingResult = "Neprovedeno";
uint32_t lastPingRttMs = 0;
bool logStorageReady = false;
bool serverStarted = false;
bool fileRefreshAttempted = false;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastFileRefreshMs = 0;
unsigned long lastUpButtonAcceptedMs = 0;
unsigned long setButtonStateChangedMs = 0;
int lastRawSetButtonState = HIGH;
int stableSetButtonState = HIGH;
volatile bool upButtonPressPending = false;
volatile bool pingFinished = false;
volatile bool pingReplyReceived = false;
volatile uint32_t pingRttMs = 0;

void logMessage(const String& message);

void IRAM_ATTR onUpButtonFalling() {
  upButtonPressPending = true;
}

void onPingSuccess(esp_ping_handle_t handle, void*) {
  uint32_t elapsedMs = 0;
  if (esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsedMs,
                           sizeof(elapsedMs)) == ESP_OK) {
    pingRttMs = elapsedMs;
    pingReplyReceived = true;
  }
}

void onPingEnd(esp_ping_handle_t, void*) {
  pingFinished = true;
}

void checkFileServerPing() {
  lastPingResult = "Testuji";
  lastPingRttMs = 0;
  pingFinished = false;
  pingReplyReceived = false;
  pingRttMs = 0;

  if (WiFi.status() != WL_CONNECTED) {
    lastPingResult = "Wi-Fi neni pripojena";
    return;
  }

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  config.count = 1;
  config.interval_ms = 100;
  config.timeout_ms = 1500;
  config.target_addr.type = IPADDR_TYPE_V4;
  config.target_addr.u_addr.ip4.addr =
      static_cast<uint32_t>(fileServerAddress);

  esp_ping_callbacks_t callbacks{};
  callbacks.on_ping_success = onPingSuccess;
  callbacks.on_ping_end = onPingEnd;

  esp_ping_handle_t handle = nullptr;
  esp_err_t error = esp_ping_new_session(&config, &callbacks, &handle);
  if (error != ESP_OK) {
    lastPingResult = "Ping nelze spustit";
    logMessage("Ping nelze spustit, chyba " + String(error) + ".");
    return;
  }

  error = esp_ping_start(handle);
  if (error != ESP_OK) {
    esp_ping_delete_session(handle);
    lastPingResult = "Ping nelze spustit";
    logMessage("Ping nelze spustit, chyba " + String(error) + ".");
    return;
  }

  const unsigned long startedMs = millis();
  while (!pingFinished && millis() - startedMs < 3000UL) {
    delay(10);
  }

  if (!pingFinished) {
    esp_ping_stop(handle);
    lastPingResult = "Ping test vyprsel";
  } else if (pingReplyReceived) {
    lastPingRttMs = pingRttMs;
    lastPingResult = "Odpovida";
  } else {
    lastPingResult = "Neodpovida (ICMP timeout)";
  }

  esp_ping_delete_session(handle);
  logMessage("Ping " + fileServerAddress.toString() + ": " + lastPingResult +
             (pingReplyReceived ? ", " + String(lastPingRttMs) + " ms." : "."));
}

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

void drawCenteredText(const String& text) {
  std::vector<String> lines;
  const int maxCharacters = (display.width() - 40) / 18;
  int start = 0;
  while (start < text.length()) {
    int end = min(start + maxCharacters, static_cast<int>(text.length()));
    if (end < text.length()) {
      const int space = text.lastIndexOf(' ', end);
      if (space > start) {
        end = space;
      }
    }
    lines.push_back(text.substring(start, end));
    start = end;
    while (start < text.length() && text[start] == ' ') {
      ++start;
    }
  }
  if (lines.empty()) {
    lines.push_back("");
  }

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(3);
    const int lineHeight = 24;
    int16_t x1;
    int16_t y1;
    uint16_t textWidth;
    uint16_t textHeight;
    display.getTextBounds("A", 0, 0, &x1, &y1, &textWidth, &textHeight);
    int y = (display.height() - lines.size() * lineHeight) / 2 - y1;

    for (const String& line : lines) {
      display.getTextBounds(line.c_str(), 0, 0, &x1, &y1, &textWidth,
                            &textHeight);
      display.setCursor((display.width() - textWidth) / 2 - x1, y);
      display.print(line);
      y += lineHeight;
    }
  } while (display.nextPage());
}

void showCurrentFileName() {
  if (cachedFiles.empty()) {
    drawCenteredText("Zadne soubory. " + noFilesReason);
    return;
  }
  drawCenteredText(cachedFiles[currentFileIndex].name);
}

void setNoFilesReason(const String& reason) {
  if (noFilesReason == reason) {
    return;
  }
  noFilesReason = reason;
  if (cachedFiles.empty() && displayMode == DisplayMode::FileName) {
    showCurrentFileName();
  }
}

void showDiagnostics() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const String ipAddress =
      connected ? WiFi.localIP().toString() : String("nedostupna");

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    display.setCursor(20, 35);
    display.print(connected ? "Wi-Fi: PRIPOJENO" : "Wi-Fi: NEPRIPOJENO");
    display.setCursor(20, 90);
    display.print("Ctecka: ");
    display.print(ipAddress);
    display.print(":");
    display.print(logServerPort);
    display.print(serverStarted ? " BEZI" : " CEKA");
    display.setCursor(20, 145);
    display.print("Souborovy server: ");
    display.print(fileServerAddress);
    display.print(":");
    display.print(fileServerPort);
    display.setCursor(20, 200);
    display.print("Posledni prikaz: ");
    display.print(lastFileRequest);
    display.setCursor(20, 255);
    display.print("Vysledek: ");
    display.print(lastFileServerResult);
    display.setCursor(20, 310);
    display.print("Prijato bajtu: ");
    display.print(lastFileResponseBytes);
    display.setCursor(20, 365);
    display.print("Trvani: ");
    display.print(lastFileRequestDurationMs);
    display.print(" ms; opakovani: 10 s");
    display.setCursor(20, 420);
    display.print("Ping: ");
    display.print(lastPingResult);
    if (lastPingResult == "Odpovida") {
      display.print(" ");
      display.print(lastPingRttMs);
      display.print(" ms");
    }
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

bool requestFileServer(const String& request, std::vector<uint8_t>& response) {
  const unsigned long requestStartedMs = millis();
  lastFileRequest = request;
  lastFileServerResult = "Spojuji se";
  lastFileResponseBytes = 0;
  lastFileRequestDurationMs = 0;

  if (WiFi.status() != WL_CONNECTED) {
    lastFileServerResult = "Wi-Fi neni pripojena";
    lastFileRequestDurationMs = millis() - requestStartedMs;
    logMessage("Souborovy server: " + lastFileServerResult);
    return false;
  }

  WiFiClient client;
  if (!client.connect(fileServerAddress, fileServerPort)) {
    lastFileServerResult = "TCP spojeni selhalo";
    lastFileRequestDurationMs = millis() - requestStartedMs;
    logMessage("Souborovy server " + fileServerAddress.toString() + ":" +
               String(fileServerPort) + " - " + lastFileServerResult);
    return false;
  }

  lastFileServerResult = "TCP spojeno, odesilam";
  const String framedRequest = request + "\n";
  const size_t requestLength = framedRequest.length();
  if (client.write(reinterpret_cast<const uint8_t*>(framedRequest.c_str()),
                   requestLength) != requestLength) {
    client.stop();
    lastFileServerResult = "Odeslani prikazu selhalo";
    lastFileRequestDurationMs = millis() - requestStartedMs;
    logMessage("Souborovy server: " + lastFileServerResult);
    return false;
  }

  lastFileServerResult = "Cekam na odpoved";
  uint8_t buffer[1024];
  unsigned long lastResponseMs = millis();
  while (millis() - lastResponseMs < tcpResponseTimeoutMs) {
    while (client.available()) {
      const int bytesRead = client.read(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        response.insert(response.end(), buffer, buffer + bytesRead);
        lastFileResponseBytes += bytesRead;
        lastResponseMs = millis();
      }
    }

    if (!client.connected() && !client.available()) {
      client.stop();
      lastFileServerResult = "Odpoved prijata";
      lastFileRequestDurationMs = millis() - requestStartedMs;
      logMessage("Souborovy server odpovedel: " +
                 String(lastFileResponseBytes) + " B za " +
                 String(lastFileRequestDurationMs) + " ms.");
      return true;
    }
    delay(1);
  }

  client.stop();
  lastFileServerResult = "Timeout, odpoved neprisla cela";
  lastFileRequestDurationMs = millis() - requestStartedMs;
  logMessage("Souborovy server: " + lastFileServerResult + ", prijato " +
             String(lastFileResponseBytes) + " B.");
  return false;
}

void refreshCachedFiles() {
  std::vector<uint8_t> listBytes;
  if (!requestFileServer("seznamobrazu.txt", listBytes)) {
    setNoFilesReason(fileServerAddress.toString() + ":" +
                     String(fileServerPort) + " " + lastFileServerResult +
                     " pri " + lastFileRequest);
    return;
  }

  listBytes.push_back(0);
  const String fileList(reinterpret_cast<const char*>(listBytes.data()));
  if (fileList.isEmpty()) {
    const bool hadFiles = !cachedFiles.empty();
    cachedFiles.clear();
    currentFileIndex = 0;
    setNoFilesReason("Server vratil prazdny seznam obraz*.* souboru");
    if (hadFiles && displayMode == DisplayMode::FileName) {
      showCurrentFileName();
    }
    logMessage("Server vratil prazdny seznam souboru.");
    return;
  }

  std::vector<CachedFile> downloadedFiles;
  int start = 0;

  while (start < fileList.length()) {
    int end = fileList.indexOf(';', start);
    if (end < 0) {
      end = fileList.length();
    }

    String fileName = fileList.substring(start, end);
    fileName.trim();
    if (!fileName.isEmpty()) {
      std::vector<uint8_t> contents;
      if (!requestFileServer("file_" + fileName, contents)) {
        logMessage("Soubor se nepodarilo stahnout: " + fileName);
        setNoFilesReason("Chyba pri " + lastFileRequest + ": " +
                         lastFileServerResult);
        return;
      }

      downloadedFiles.push_back(
          CachedFile{fileName, std::move(contents)});
    }
    start = end + 1;
  }

  String previousFileName;
  if (!cachedFiles.empty()) {
    previousFileName = cachedFiles[currentFileIndex].name;
  }

  const bool hadFiles = !cachedFiles.empty();
  cachedFiles = std::move(downloadedFiles);
  currentFileIndex = 0;
  setNoFilesReason("Server nema zadne soubory obraz*.*");
  for (size_t i = 0; i < cachedFiles.size(); ++i) {
    if (cachedFiles[i].name == previousFileName) {
      currentFileIndex = i;
      break;
    }
  }

  if (((cachedFiles.empty() && hadFiles) ||
       (!cachedFiles.empty() &&
        cachedFiles[currentFileIndex].name != previousFileName)) &&
      displayMode == DisplayMode::FileName) {
    showCurrentFileName();
  }
  logMessage("Nacteno souboru do pameti: " + String(cachedFiles.size()));
}

void refreshFilesIfDue() {
  if (WiFi.status() != WL_CONNECTED) {
    if (cachedFiles.empty()) {
      setNoFilesReason("Wi-Fi neni pripojena, nelze kontaktovat server");
    }
    return;
  }

  const unsigned long now = millis();
  if (fileRefreshAttempted &&
      now - lastFileRefreshMs < fileRefreshIntervalMs) {
    return;
  }

  fileRefreshAttempted = true;
  refreshCachedFiles();
  lastFileRefreshMs = millis();
}

bool buttonPressed(uint8_t pin, int& lastRawState, int& stableState,
                   unsigned long& stateChangedMs) {
  const int reading = digitalRead(pin);
  const unsigned long now = millis();

  if (reading != lastRawState) {
    lastRawState = reading;
    stateChangedMs = now;
  }

  if (now - stateChangedMs >= buttonDebounceMs && reading != stableState) {
    stableState = reading;
    return stableState == LOW;
  }
  return false;
}

void handleUpButton() {
  noInterrupts();
  const bool pressed = upButtonPressPending;
  upButtonPressPending = false;
  interrupts();
  const unsigned long now = millis();
  if (!pressed || now - lastUpButtonAcceptedMs < buttonDebounceMs) {
    return;
  }
  lastUpButtonAcceptedMs = now;

  logMessage("Stisknuto tlacitko UP.");
  if (!cachedFiles.empty()) {
    currentFileIndex = (currentFileIndex + 1) % cachedFiles.size();
  }
  displayMode = DisplayMode::FileName;
  showCurrentFileName();
}

void handleSetButton() {
  if (buttonPressed(setButtonPin, lastRawSetButtonState, stableSetButtonState,
                    setButtonStateChangedMs)) {
    checkFileServerPing();
    displayMode = DisplayMode::Diagnostics;
    showDiagnostics();
  }
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
  pinMode(upButtonPin, INPUT);
  pinMode(setButtonPin, INPUT);
  lastRawSetButtonState = digitalRead(setButtonPin);
  stableSetButtonState = lastRawSetButtonState;
  lastUpButtonAcceptedMs = millis() - buttonDebounceMs;
  attachInterrupt(digitalPinToInterrupt(upButtonPin), onUpButtonFalling,
                  FALLING);
  delay(100);

  SPI.begin(epdSck, 13, epdMosi, epdCs);
  display.init();
  display.setRotation(1);
  display.setFullWindow();
  showCurrentFileName();

  startWifi();
}

void loop() {
  maintainWifiAndServer();
  handleTcpClient();
  refreshFilesIfDue();
  handleUpButton();
  handleSetButton();

  delay(1);
}
