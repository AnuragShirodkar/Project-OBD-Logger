// Spare Teensy 4.1 bench demo. OLED GND/GND, VCC/3.3V, SDA/18, SCL/19.
// Install Adafruit SSD1306 and its dependencies in Arduino Library Manager.
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 32, &Wire, -1);

enum Pattern : uint8_t { Off, On, Slow, Fast, ThreeQuick, ReplyPulse };
struct Stage {
  const char *status;
  const char *diagnostic;
  Pattern icons[5];  // READY, RECORD, USB, ALERT, onboard activity
};

const Stage stages[] = {
  {"Waiting for ECU", "Discover 01 00", {Slow}},
  {"ECU ready", "PID map  01 00", {On, Off, Off, Off, ReplyPulse}},
  {"Recording", "RPM PID  01 0C", {On, On}},
  {"Reply received", "RPM RX   41 0C", {On, On, Off, Off, ReplyPulse}},
  {"Mode 04 held", "Clear DTCs  04", {On, On, Off, Off, On}},
  {"Clear queued", "Clear DTCs  04", {On, Fast}},
  {"Clear sent", "Clear TX    04", {On, Fast}},
  {"Clear confirmed", "Clear RX    44", {On, Slow}},
  {"Clear unconfirmed", "Check clear 04", {On, Off, Off, ThreeQuick}},
  {"Saving log", "OBD log to SD", {Off, Fast}},
  {"Log closed", "OBD log closed", {}},
  {"USB file access", "Browse OBD log", {Off, Off, On}},
  {"SD write fault", "OBD log error", {Off, Off, Off, On, Fast}}
};

// One byte per row: signal bars, SD card, USB, warning triangle, envelope.
const uint8_t glyphs[][8] = {
  {0x01,0x01,0x05,0x05,0x15,0x15,0x55,0x55},
  {0x3e,0x6a,0xca,0x82,0xba,0xba,0x82,0xfe},
  {0x10,0x38,0x10,0x52,0x54,0x38,0x10,0x38},
  {0x18,0x18,0x24,0x24,0x52,0x42,0x99,0xff},
  {0x00,0xff,0xc3,0xa5,0x99,0x81,0xff,0x00}
};

bool lit(Pattern pattern, unsigned ms) {
  switch (pattern) {
    case On: return true;
    case Slow: return ms / 500 % 2 == 0;
    case Fast: return ms / 250 % 2 == 0;
    case ThreeQuick: return ms < 900 && ms / 150 % 2 == 0;
    case ReplyPulse: return ms % 1000 < 150;
    default: return false;
  }
}
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
  const uint32_t elapsed = millis();
  const size_t stageCount = sizeof(stages) / sizeof(stages[0]);
  const Stage &stage = stages[(elapsed / 3000) % stageCount];
  const Pattern ready = stage.icons[0];
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(ready == On ? "ECU LINKED" : ready == Slow ? "ECU WAIT" : "ECU IDLE");
  oled.setCursor(74, 0);
  oled.print("500 kbps");

  oled.setCursor(0, 8);
  oled.print("ISO 15765-4 11b  DEMO");
  oled.setCursor(0, 16);
  oled.print(stage.diagnostic);
  for (unsigned i = 0; i < 5; ++i) {
    if (lit(stage.icons[i], elapsed % 3000)) {
      oled.drawBitmap(84 + i * 9, 16, glyphs[i], 8, 8, SSD1306_WHITE);
    }
  }
  oled.setCursor(0, 24);
  oled.print("LOG ");
  oled.print(stage.status);
  oled.display();
  delay(50);
}
