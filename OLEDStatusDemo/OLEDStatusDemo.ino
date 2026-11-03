// Spare Teensy 4.1 bench demo. OLED GND/GND, VCC/3.3V, SDA/18, SCL/19.
// Install Adafruit SSD1306 and its dependencies in Arduino Library Manager.
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 32, &Wire, -1);

struct Stage {
  const char *status;
  const char *mode;
  bool linked;
  unsigned framesPerSecond;  // Simulated bus traffic.
  unsigned sdUsedPercent;
};

const Stage stages[] = {
  {"Waiting for ECU", "01", false, 18, 74},
  {"ECU connected", "01", true, 126, 74},
  {"OBD logging", "01", true, 342, 75},
  {"ECU reply received", "01", true, 356, 75},
  {"Clear button held", "01", true, 321, 75},
  {"Clear queued", "01", true, 318, 75},
  {"Clear sent", "04", true, 308, 75},
  {"Clear confirmed", "04", true, 312, 75},
  {"Clear unconfirmed", "04", true, 298, 75},
  {"Saving log", "--", false, 0, 75},
  {"Log completed", "--", false, 0, 75},
  {"USB file access", "--", false, 0, 75},
  {"Error: SD write", "--", false, 0, 75}
};

// Two horizontal chain links, joined or separated at the center.
void drawLink(bool linked) {
  int gap = linked ? 0 : 4;
  oled.drawRoundRect(0, 1, 7, 5, 2, SSD1306_WHITE);
  oled.drawRoundRect(5 + gap, 3, 7, 5, 2, SSD1306_WHITE);
  if (linked) oled.drawLine(4, 3, 8, 5, SSD1306_WHITE);
}

const uint8_t sdIcon[] = {0x3e,0x6a,0xca,0x82,0xba,0xba,0x82,0xfe};
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
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  drawLink(stage.linked);
  oled.setCursor(18, 0);
  oled.print("J1979");
  oled.setCursor(51, 0);
  oled.print("Mode-");
  oled.print(stage.mode);
  oled.drawBitmap(97, 0, sdIcon, 8, 8, SSD1306_WHITE);
  oled.setCursor(108, 0);
  oled.print(stage.sdUsedPercent);
  oled.print('%');

  // Simulate a changing frame rate, using 400 frames/s as the demo scale.
  const int phase = (elapsed / 200) % 12;
  const int wave = (phase <= 6 ? phase : 12 - phase) - 3;
  const unsigned rate = stage.framesPerSecond * (100 + 4 * wave) / 100;
  const unsigned fill = min(rate, 400u) * 126 / 400;
  oled.drawRect(0, 9, 128, 4, SSD1306_WHITE);
  if (fill) oled.fillRect(1, 10, fill, 2, SSD1306_WHITE);
  char traffic[22];
  snprintf(traffic, sizeof(traffic), "Traffic %u frames/s", rate);
  oled.setCursor((128 - 6 * strlen(traffic)) / 2, 13);
  oled.print(traffic);
  oled.setCursor(0, 24);
  oled.print(stage.status);
  oled.display();
  delay(100);
}
