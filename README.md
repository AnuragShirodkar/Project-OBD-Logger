# Project OBD Logger

The active prototype is a **Teensy 4.1 manual OBD tester for a spare Audi ECU**.
Enter requests through Arduino IDE Serial Monitor; view responses there and
record the exchange to the Teensy's SD card as an OBD-SkillLogger text report.

- [Teensy sketch, commands and first bench test](teensy-obd-tester/README.md)
- [Open the Arduino sketch](teensy-obd-tester/TeensyOBDTester/TeensyOBDTester.ino)

Hardware: Teensy 4.1 with two 8 MB PSRAM chips and TLE9371VSJ. CAN1 uses TX 22,
RX 23; STB is grounded. The prototype does not require PSRAM.

Current scope: passive CAN bitrate detection with manual override, 11-/29-bit
address selection, manual OBD requests, multi-frame responses, negative responses
and distinct timeouts, USB display and SD logging. No physical ECU test or firmware
upload has been performed as part of this development.

Later: a Windows configuration tool exporting multi-mode OBD JSON, transferred by
USB or SD. Android is no longer planned. XCP and CCP remain deferred.

## Earlier simulator work

The [STM32F407 four-ECU simulator](stm32-obd-simulator/README.md) and
[its wiring guide](stm32-obd-simulator/WIRING.md) are preserved for reference.
The current test plan uses the real ECU directly.

See each component's README for validation and limitations. Build artifacts, local
tool dependencies and credentials must not be committed.
