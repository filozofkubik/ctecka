#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_4C.h>

namespace {

constexpr uint8_t epdMosi = 11;
constexpr uint8_t epdSck = 12;
constexpr uint8_t epdCs = 10;
constexpr uint8_t epdDc = 48;
constexpr uint8_t epdRst = 45;
constexpr uint8_t epdBusy = 38;
constexpr uint8_t peripheralPower = 47;

GxEPD2_4C<GxEPD2_750c_GDEM075F52,
          GxEPD2_750c_GDEM075F52::HEIGHT / 2>
    display(GxEPD2_750c_GDEM075F52(epdCs, epdDc, epdRst, epdBusy));

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

  Serial.println("Na displeji je Ahoj!");
}

void loop() {}
