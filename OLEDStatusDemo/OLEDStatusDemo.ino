// Spare Teensy 4.1 bench demo. OLED GND/GND, VCC/3.3V, SDA/18, SCL/19.
// Install Adafruit SSD1306 and its dependencies in Arduino Library Manager.
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 32, &Wire, -1);

enum Pattern : uint8_t { Off, On, Slow, Fast, ThreeQuick, ReplyPulse };
struct Stage {
  const char *status;
  uint8_t framesPerTick;
  Pattern icons[5];  // READY, RECORD, USB, ALERT, onboard activity
};

const Stage stages[] = {
  {"Waiting for ECU", 0, {Slow}},
  {"ECU ready", 3, {On, Off, Off, Off, ReplyPulse}},
  {"Recording", 8, {On, On}},
  {"Reply received", 12, {On, On, Off, Off, ReplyPulse}},
  {"Mode 04 held", 5, {On, On, Off, Off, On}},
  {"Clear queued", 6, {On, Fast}},
  {"Clear sent", 4, {On, Fast}},
  {"Clear confirmed", 9, {On, Slow}},
  {"Clear unconfirmed", 7, {On, Off, Off, ThreeQuick}},
  {"Saving log", 0, {Off, Fast}},
  {"Log closed", 0, {}},
  {"USB file access", 0, {Off, Off, On}},
  {"SD write fault", 0, {Off, Off, Off, On, Fast}}
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
  const uint32_t tick = elapsed / 250;
  static uint32_t previousTick = UINT32_MAX;
  static uint32_t frames = 0;
  const size_t stageCount = sizeof(stages) / sizeof(stages[0]);
  const size_t index = (tick / 12) % stageCount;
  const Stage &stage = stages[index];
  if (index == 0) frames = 0;
  const unsigned activity = stage.framesPerTick ? stage.framesPerTick + tick % 3 : 0;
  if (tick != previousTick) frames += activity;
  previousTick = tick;

  // Reference-style simulated date/time, restarting with each demo session.
  const unsigned seconds = 11 * 3600 + 24 * 60 + 32 + (tick / 4) % (stageCount * 3);
  char clockText[16];
  snprintf(clockText, sizeof(clockText), "16/04 %02u:%02u:%02u",
           seconds / 3600, seconds / 60 % 60, seconds % 60);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(clockText);
  for (unsigned i = 0; i < 5; ++i) {
    if (lit(stage.icons[i], elapsed % 3000)) oled.drawBitmap(84 + i * 9, 0, glyphs[i], 8, 8, SSD1306_WHITE);
  }
  oled.setCursor(72, 10);
  oled.print("DM");  // Bench demo marker

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

  delay(50);
}
