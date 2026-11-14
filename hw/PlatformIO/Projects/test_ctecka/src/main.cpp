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
constexpr uint8_t downButtonPin = 41;

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
  uint64_t contentHash;
};

enum class DisplayMode {
  FileName,
  Diagnostics,
  Sleeping,
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
unsigned long upButtonStateChangedMs = 0;
unsigned long setButtonStateChangedMs = 0;
unsigned long downButtonStateChangedMs = 0;
int lastRawUpButtonState = HIGH;
int stableUpButtonState = HIGH;
int lastRawSetButtonState = HIGH;
int stableSetButtonState = HIGH;
int lastRawDownButtonState = HIGH;
int stableDownButtonState = HIGH;
volatile bool pingFinished = false;
volatile bool pingReplyReceived = false;
volatile uint32_t pingRttMs = 0;

void logMessage(const String& message);

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

uint16_t readBmpUInt16(const std::vector<uint8_t>& contents, size_t offset) {
  return static_cast<uint16_t>(contents[offset]) |
         (static_cast<uint16_t>(contents[offset + 1]) << 8);
}

uint32_t readBmpUInt32(const std::vector<uint8_t>& contents, size_t offset) {
  return static_cast<uint32_t>(contents[offset]) |
         (static_cast<uint32_t>(contents[offset + 1]) << 8) |
         (static_cast<uint32_t>(contents[offset + 2]) << 16) |
         (static_cast<uint32_t>(contents[offset + 3]) << 24);
}

struct BmpInfo {
  uint32_t pixelOffset;
  uint32_t width;
  uint32_t height;
  size_t rowSize;
  bool topDown;
};

bool readBmpInfo(const std::vector<uint8_t>& contents, BmpInfo& info) {
  if (contents.size() < 54 || contents[0] != 'B' || contents[1] != 'M') {
    return false;
  }

  const uint32_t dibSize = readBmpUInt32(contents, 14);
  const int32_t width = static_cast<int32_t>(readBmpUInt32(contents, 18));
  const int32_t signedHeight =
      static_cast<int32_t>(readBmpUInt32(contents, 22));
  const uint32_t pixelOffset = readBmpUInt32(contents, 10);
  if (dibSize < 40 || width <= 0 || signedHeight == 0 ||
      signedHeight == INT32_MIN || readBmpUInt16(contents, 26) != 1 ||
      readBmpUInt16(contents, 28) != 24 ||
      readBmpUInt32(contents, 30) != 0 ||
      static_cast<uint64_t>(pixelOffset) < uint64_t(14) + dibSize ||
      pixelOffset > contents.size()) {
    return false;
  }

  const uint32_t height = signedHeight < 0
                              ? static_cast<uint32_t>(-signedHeight)
                              : static_cast<uint32_t>(signedHeight);
  const uint64_t rowSize =
      (static_cast<uint64_t>(width) * 3 + 3) & ~uint64_t(3);
  const uint64_t endOffset =
      static_cast<uint64_t>(pixelOffset) + rowSize * height;
  if (endOffset > contents.size()) {
    return false;
  }

  info = {pixelOffset, static_cast<uint32_t>(width), height,
          static_cast<size_t>(rowSize), signedHeight < 0};
  return true;
}

uint16_t bmpColor(uint8_t red, uint8_t green, uint8_t blue) {
  const uint16_t colors[] = {GxEPD_BLACK, GxEPD_WHITE, GxEPD_RED,
                             GxEPD_YELLOW};
  const int palette[][3] = {{0, 0, 0},       {255, 255, 255},
                            {255, 0, 0},     {255, 255, 0}};
  uint32_t bestDistance = UINT32_MAX;
  uint16_t bestColor = GxEPD_BLACK;
  for (size_t i = 0; i < 4; ++i) {
    const int redDifference = red - palette[i][0];
    const int greenDifference = green - palette[i][1];
    const int blueDifference = blue - palette[i][2];
    const uint32_t distance =
        redDifference * redDifference + greenDifference * greenDifference +
        blueDifference * blueDifference;
    if (distance < bestDistance) {
      bestDistance = distance;
      bestColor = colors[i];
    }
  }
  return bestColor;
}

void drawBmp(const std::vector<uint8_t>& contents, const BmpInfo& info) {
  const uint32_t screenWidth = display.width();
  const uint32_t screenHeight = display.height();
  uint32_t imageWidth = info.width;
  uint32_t imageHeight = info.height;

  if (imageWidth > screenWidth || imageHeight > screenHeight) {
    if (static_cast<uint64_t>(imageWidth) * screenHeight >
        static_cast<uint64_t>(imageHeight) * screenWidth) {
      imageHeight = static_cast<uint64_t>(imageHeight) * screenWidth /
                    imageWidth;
      imageWidth = screenWidth;
    } else {
      imageWidth = static_cast<uint64_t>(imageWidth) * screenHeight /
                   imageHeight;
      imageHeight = screenHeight;
    }
    imageWidth = max(imageWidth, uint32_t(1));
    imageHeight = max(imageHeight, uint32_t(1));
  }

  const uint32_t left = (screenWidth - imageWidth) / 2;
  const uint32_t top = (screenHeight - imageHeight) / 2;
  for (uint32_t y = 0; y < imageHeight; ++y) {
    const uint32_t sourceY =
        static_cast<uint64_t>(y) * info.height / imageHeight;
    const uint32_t row = info.topDown ? sourceY : info.height - 1 - sourceY;
    for (uint32_t x = 0; x < imageWidth; ++x) {
      const uint32_t sourceX =
          static_cast<uint64_t>(x) * info.width / imageWidth;
      const size_t pixelOffset =
          info.pixelOffset + static_cast<size_t>(row) * info.rowSize +
          static_cast<size_t>(sourceX) * 3;
      display.drawPixel(
          left + x, top + y,
          bmpColor(contents[pixelOffset + 2], contents[pixelOffset + 1],
                   contents[pixelOffset]));
    }
  }
}

uint16_t textColor(String colorName) {
  colorName.toLowerCase();
  if (colorName == "red") {
    return GxEPD_RED;
  }
  if (colorName == "yellow") {
    return GxEPD_YELLOW;
  }
  if (colorName == "white") {
    return GxEPD_WHITE;
  }
  return GxEPD_BLACK;
}

enum class CzechAccent {
  None,
  Acute,
  Caron,
  Ring,
};

bool mapCzechCharacter(uint32_t codePoint, char& base, CzechAccent& accent) {
  switch (codePoint) {
    case 0x00C1: base = 'A'; accent = CzechAccent::Acute; return true;
    case 0x00E1: base = 'a'; accent = CzechAccent::Acute; return true;
    case 0x00C9: base = 'E'; accent = CzechAccent::Acute; return true;
    case 0x00E9: base = 'e'; accent = CzechAccent::Acute; return true;
    case 0x00CD: base = 'I'; accent = CzechAccent::Acute; return true;
    case 0x00ED: base = 'i'; accent = CzechAccent::Acute; return true;
    case 0x00D3: base = 'O'; accent = CzechAccent::Acute; return true;
    case 0x00F3: base = 'o'; accent = CzechAccent::Acute; return true;
    case 0x00DA: base = 'U'; accent = CzechAccent::Acute; return true;
    case 0x00FA: base = 'u'; accent = CzechAccent::Acute; return true;
    case 0x00DD: base = 'Y'; accent = CzechAccent::Acute; return true;
    case 0x00FD: base = 'y'; accent = CzechAccent::Acute; return true;
    case 0x010C: base = 'C'; accent = CzechAccent::Caron; return true;
    case 0x010D: base = 'c'; accent = CzechAccent::Caron; return true;
    case 0x010E: base = 'D'; accent = CzechAccent::Caron; return true;
    case 0x010F: base = 'd'; accent = CzechAccent::Caron; return true;
    case 0x011A: base = 'E'; accent = CzechAccent::Caron; return true;
    case 0x011B: base = 'e'; accent = CzechAccent::Caron; return true;
    case 0x0147: base = 'N'; accent = CzechAccent::Caron; return true;
    case 0x0148: base = 'n'; accent = CzechAccent::Caron; return true;
    case 0x0158: base = 'R'; accent = CzechAccent::Caron; return true;
    case 0x0159: base = 'r'; accent = CzechAccent::Caron; return true;
    case 0x0160: base = 'S'; accent = CzechAccent::Caron; return true;
    case 0x0161: base = 's'; accent = CzechAccent::Caron; return true;
    case 0x0164: base = 'T'; accent = CzechAccent::Caron; return true;
    case 0x0165: base = 't'; accent = CzechAccent::Caron; return true;
    case 0x017D: base = 'Z'; accent = CzechAccent::Caron; return true;
    case 0x017E: base = 'z'; accent = CzechAccent::Caron; return true;
    case 0x016E: base = 'U'; accent = CzechAccent::Ring; return true;
    case 0x016F: base = 'u'; accent = CzechAccent::Ring; return true;
    default: return false;
  }
}

uint32_t nextUtf8CodePoint(const String& text, size_t& position) {
  const uint8_t first = static_cast<uint8_t>(text[position++]);
  if (first < 0x80) {
    return first;
  }

  uint8_t extraBytes;
  uint32_t codePoint;
  if (first >= 0xC2 && first <= 0xDF) {
    extraBytes = 1;
    codePoint = first & 0x1F;
  } else if (first >= 0xE0 && first <= 0xEF) {
    extraBytes = 2;
    codePoint = first & 0x0F;
  } else if (first >= 0xF0 && first <= 0xF4) {
    extraBytes = 3;
    codePoint = first & 0x07;
  } else {
    return '?';
  }

  if (position + extraBytes > text.length()) {
    return '?';
  }
  for (uint8_t i = 0; i < extraBytes; ++i) {
    const uint8_t next = static_cast<uint8_t>(text[position++]);
    if ((next & 0xC0) != 0x80) {
      return '?';
    }
    codePoint = (codePoint << 6) | (next & 0x3F);
  }

  if ((extraBytes == 2 && codePoint < 0x800) ||
      (extraBytes == 3 && codePoint < 0x10000) ||
      (codePoint >= 0xD800 && codePoint <= 0xDFFF) || codePoint > 0x10FFFF) {
    return '?';
  }
  return codePoint;
}

void drawCzechAccent(int16_t x, int16_t y, uint8_t size, CzechAccent accent,
                     uint16_t color) {
  const int16_t centerX = x + 3 * size;
  const int16_t topY = y - 4 * size;
  if (accent == CzechAccent::Acute) {
    display.drawLine(centerX - size, topY + 2 * size, centerX + size, topY,
                     color);
  } else if (accent == CzechAccent::Caron) {
    display.drawLine(centerX - 2 * size, topY, centerX, topY + 2 * size,
                     color);
    display.drawLine(centerX, topY + 2 * size, centerX + 2 * size, topY,
                     color);
  } else if (accent == CzechAccent::Ring) {
    display.drawCircle(centerX, topY + size, size, color);
  }
}

void drawUtf8Text(const String& text, uint8_t size, uint16_t color) {
  size_t position = 0;
  while (position < text.length()) {
    const uint32_t codePoint = nextUtf8CodePoint(text, position);
    if (codePoint < 0x80) {
      display.write(static_cast<uint8_t>(codePoint));
      continue;
    }

    char base;
    CzechAccent accent = CzechAccent::None;
    if (!mapCzechCharacter(codePoint, base, accent)) {
      display.write('?');
      continue;
    }

    const int16_t x = display.getCursorX();
    const int16_t y = display.getCursorY();
    display.write(static_cast<uint8_t>(base));
    if (base == 'i' && accent == CzechAccent::Acute) {
      display.fillRect(x + 2 * size, y, size, size, GxEPD_WHITE);
    }
    drawCzechAccent(x, y, size, accent, color);
  }
}

void drawTextRecords(const std::vector<uint8_t>& contents) {
  size_t position = 0;
  while (position < contents.size()) {
    String fields[6];
    bool completeRecord = true;
    for (String& field : fields) {
      size_t end = position;
      while (end < contents.size() && contents[end] != ';') {
        ++end;
      }
      if (end == contents.size()) {
        completeRecord = false;
        break;
      }
      while (position < end) {
        field += static_cast<char>(contents[position++]);
      }
      field.trim();
      ++position;
    }

    if (!completeRecord) {
      return;
    }
    if (fields[0] != "T") {
      continue;
    }

    const int pixelHeight = constrain(fields[3].toInt(), 1, 160);
    const uint8_t textSize = (pixelHeight + 7) / 8;
    const uint16_t color = textColor(fields[4]);
    display.setTextSize(textSize);
    display.setTextColor(color);
    display.setCursor(fields[1].toInt(), fields[2].toInt());
    drawUtf8Text(fields[5], textSize, color);
  }
}

void showCurrentFileName() {
  if (cachedFiles.empty()) {
    drawCenteredText("Zadne soubory. " + noFilesReason);
    return;
  }

  const CachedFile& file = cachedFiles[currentFileIndex];
  String fileName = file.name;
  fileName.toLowerCase();

  if (fileName.endsWith(".txt")) {
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.setTextWrap(false);
      drawTextRecords(file.contents);
    } while (display.nextPage());
    return;
  }

  if (fileName.endsWith(".bmp")) {
    BmpInfo info{};
    if (!readBmpInfo(file.contents, info)) {
      drawCenteredText("BMP nepodporen. Pouzijte 24bit BMP bez komprese.");
      return;
    }
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      drawBmp(file.contents, info);
    } while (display.nextPage());
    return;
  }

  if (fileName.endsWith(".png")) {
    drawCenteredText("PNG nepodporuji. Pouzijte 24bit BMP bez komprese.");
    return;
  }

  drawCenteredText("Nepodporovany format. Pouzijte TXT nebo BMP.");
}

void showSleepingMessage() {
  constexpr uint8_t textSize = 3;
  constexpr int textWidth = 4 * 6 * textSize;
  constexpr int textHeight = 8 * textSize;

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextWrap(false);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(textSize);
    display.setCursor((display.width() - textWidth) / 2,
                      (display.height() - textHeight) / 2);
    drawUtf8Text("spím", textSize, GxEPD_BLACK);
  } while (display.nextPage());
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

uint64_t hashContents(const std::vector<uint8_t>& contents) {
  uint64_t hash = 14695981039346656037ULL;
  for (uint8_t byte : contents) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
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

      const uint64_t contentHash = hashContents(contents);
      downloadedFiles.push_back(
          CachedFile{fileName, std::move(contents), contentHash});
    }
    start = end + 1;
  }

  String previousFileName;
  bool selectedFileContentChanged = false;
  if (!cachedFiles.empty()) {
    const CachedFile& previousFile = cachedFiles[currentFileIndex];
    previousFileName = previousFile.name;
    for (const CachedFile& downloadedFile : downloadedFiles) {
      if (downloadedFile.name == previousFileName) {
        selectedFileContentChanged =
            downloadedFile.contentHash != previousFile.contentHash;
        break;
      }
    }
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

  if ((selectedFileContentChanged ||
       (cachedFiles.empty() && hadFiles) ||
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
  if (!buttonPressed(upButtonPin, lastRawUpButtonState, stableUpButtonState,
                     upButtonStateChangedMs)) {
    return;
  }

  logMessage("Stisknuto tlacitko UP.");
  if (!cachedFiles.empty()) {
    currentFileIndex = (currentFileIndex + 1) % cachedFiles.size();
  }
  displayMode = DisplayMode::FileName;
  showCurrentFileName();
}

void handleDownButton() {
  if (buttonPressed(downButtonPin, lastRawDownButtonState,
                    stableDownButtonState, downButtonStateChangedMs)) {
    logMessage("Stisknuto tlacitko DOWN.");
    displayMode = DisplayMode::Sleeping;
    showSleepingMessage();
  }
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
  pinMode(downButtonPin, INPUT);
  lastRawUpButtonState = digitalRead(upButtonPin);
  stableUpButtonState = lastRawUpButtonState;
  lastRawSetButtonState = digitalRead(setButtonPin);
  stableSetButtonState = lastRawSetButtonState;
  lastRawDownButtonState = digitalRead(downButtonPin);
  stableDownButtonState = lastRawDownButtonState;
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
  handleDownButton();
  handleSetButton();

  delay(1);
}
