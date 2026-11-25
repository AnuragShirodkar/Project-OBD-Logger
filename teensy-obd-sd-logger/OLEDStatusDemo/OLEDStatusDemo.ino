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
  {"FIND ECU", "01", false, 18, 74},
  {"ECU READY", "01", true, 126, 74},
  {"RECORDING", "01", true, 342, 75},
  {"ECU REPLY", "01", true, 356, 75},
  {"HOLD CLEAR", "01", true, 321, 75},
  {"QUEUED", "01", true, 318, 75},
  {"CLEAR SENT", "04", true, 308, 75},
  {"CLEAR OK", "04", true, 312, 75},
  {"UNVERIFIED", "04", true, 298, 75},
  {"SAVING", "--", false, 0, 75},
  {"LOG CLOSED", "--", false, 0, 75},
  {"USB FILES", "--", false, 0, 75},
  {"SD ERROR", "--", false, 0, 75}
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
  oled.setTextSize(1);
  drawLink(stage.linked);
  oled.setCursor(18, 0);
  oled.print("J1979");
  oled.fillRoundRect(54, 0, 34, 9, 2, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(56, 1);
  oled.print("M:");
  oled.print(stage.mode);
  oled.setTextColor(SSD1306_WHITE);
  oled.drawBitmap(97, 0, sdIcon, 8, 8, SSD1306_WHITE);
  oled.setCursor(108, 0);
  oled.print(stage.sdUsedPercent);
  oled.print('%');

  // Simulate a changing frame rate, using 400 frames/s as the demo scale.
  const int phase = (elapsed / 200) % 12;
  const int wave = (phase <= 6 ? phase : 12 - phase) - 3;
  const unsigned rate = stage.framesPerSecond * (100 + 4 * wave) / 100;
  // The status is the focal point. Keep every title within ten large glyphs.
  oled.setTextSize(2);
  oled.setCursor((128 - 12 * strlen(stage.status)) / 2, 10);
  oled.print(stage.status);

  // Rising segments form a small instrument scale; dim baseline dots stay
  // visible at rest. Each illuminated segment represents 25 frames/s.
  const unsigned segments = (min(rate, 400u) + 24) / 25;
  for (unsigned i = 0; i < 16; ++i) {
    const int x = i * 5;
    const int height = 2 + i / 3;
    if (i < segments) oled.fillRect(x, 32 - height, 4, height, SSD1306_WHITE);
    else oled.drawPixel(x + 1, 31, SSD1306_WHITE);
  }
  oled.setTextSize(1);
  oled.setCursor(86, 25);
  oled.print(rate);
  oled.print(" f/s");
  oled.display();
  delay(100);
}
