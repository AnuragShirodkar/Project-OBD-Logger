// Spare Teensy 4.1 bench demo. OLED GND/GND, VCC/3.3V, SDA/18, SCL/19.
// Install Adafruit SSD1306 and its dependencies in Arduino Library Manager.
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 32, &Wire, -1);

struct Screen {
  const char *title;
  const char *detail;
};

const Screen screens[] = {
  {"WAIT ECU", "Awaiting first reply"},
  {"READY", "ECU reply validated"},
  {"RECORDING", "Ready | SD log open"},
  {"REPLY", "Matching ECU reply"},
  {"MODE 04", "Button held | 2s"},
  {"QUEUED", "Clear request queued"},
  {"CLEAR SENT", "Awaiting ECU replies"},
  {"CONFIRMED", "All ECUs accepted"},
  {"CHECK ECUS", "Clear not confirmed"},
  {"SAVING", "Closing SD log"},
  {"COMPLETE", "Log closed"},
  {"USB FILES", "SD available via MTP"},
  {"FAULT", "SD write failed"}
};

void setup() {
  Serial.begin(115200);
  Wire.begin();
  uint8_t address = 0x3C;
  for (; address <= 0x3D; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) break;
  }
  if (address > 0x3D || !oled.begin(SSD1306_SWITCHCAPVCC, address)) {
    while (true) {
      Serial.println("OLED unavailable. Check 3.3V, GND, SDA 18 and SCL 19.");
      delay(2000);
    }
  }
  oled.setTextWrap(false);
}

void loop() {
  for (const Screen &screen : screens) {
    oled.clearDisplay();
    oled.fillRect(0, 0, 128, 8, SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(1, 0);
    oled.print("OBD LOGGER      DEMO");
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 8);
    oled.print(screen.title);
    oled.setTextSize(1);
    oled.setCursor(0, 24);
    oled.print(screen.detail);
    oled.display();
    delay(3000);
  }
}
