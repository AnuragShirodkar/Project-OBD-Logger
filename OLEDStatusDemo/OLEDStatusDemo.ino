// Spare Teensy 4.1 bench demo. OLED GND/GND, VCC/3.3V, SDA/18, SCL/19.
// Install Adafruit SSD1306 and its dependencies in Arduino Library Manager.
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 32, &Wire, -1);

struct Stage {
  const char *status;
  uint8_t framesPerTick;  // Simulated CAN activity per quarter second.
};

const Stage stages[] = {
  {"Waiting for ECU", 0}, {"ECU ready", 3}, {"Recording", 8},
  {"Reply received", 12}, {"Mode 04 held", 5}, {"Clear queued", 6},
  {"Clear sent", 4}, {"Clear confirmed", 9}, {"Clear unconfirmed", 7},
  {"Saving log", 0}, {"Log closed", 0}, {"USB file access", 0},
  {"SD write fault", 0}
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
  static uint32_t tick = 0;
  static uint32_t frames = 0;
  const size_t stageCount = sizeof(stages) / sizeof(stages[0]);
  const size_t index = (tick / 12) % stageCount;
  const Stage &stage = stages[index];
  if (index == 0) frames = 0;
  const unsigned activity = stage.framesPerTick ? stage.framesPerTick + tick % 3 : 0;
  frames += activity;

  // Reference-style simulated date/time, restarting with each demo session.
  const unsigned seconds = 11 * 3600 + 24 * 60 + 32 + (tick / 4) % (stageCount * 3);
  char clockText[16];
  snprintf(clockText, sizeof(clockText), "16/04 %02u:%02u:%02u",
           seconds / 3600, seconds / 60 % 60, seconds % 60);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(clockText);
  oled.setCursor(104, 0);
  oled.print("DEMO");

  oled.setCursor(0, 10);
  oled.print("CAN1   500k");
  char countText[12];
  snprintf(countText, sizeof(countText), "%lu", (unsigned long)frames);
  oled.setCursor(128 - 6 * strlen(countText), 10);
  oled.print(countText);
  oled.drawRect(0, 18, 128, 4, SSD1306_WHITE);
  oled.fillRect(1, 19, activity * 9, 2, SSD1306_WHITE);

  oled.setCursor(0, 24);
  oled.print("LOG ");
  oled.print(stage.status);
  oled.display();
  ++tick;
  delay(250);
}
